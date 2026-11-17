"""Selected media cannot be silently regenerated, substituted or staged across namespaces."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys
from unittest import mock
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import homepage_media as selected
import docs_asset_stage as stage
import hydrate_docs_video_assets as hydration
import check_docs_video_assets as availability


def snapshot(tmp_path, monkeypatch):
    monkeypatch.setattr(selected,'ROOT',tmp_path)
    products=tmp_path/'selected';monkeypatch.setattr(selected,'SELECTED_PRODUCTS',products)
    examples=[]
    for name in ['showcases_protein','showcases_point_cloud','showcases_cortical_activity','showcases_textured_planet']:
        assets=[]
        variants=[('primary','.av1.mp4','av1'),('fallback','.mp4','h264'),('poster','.poster.webp','webp')] if name.endswith('point_cloud') else [('primary','.mp4','h264'),('poster','.poster.webp','webp')]
        for role,suffix,codec in variants:
            content=(name+role).encode();path='showcases/'+name+suffix
            p=products/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(content)
            assets.append(dict(path=path,role=role,codec=codec,bytes=len(content),sha256=hashlib.sha256(content).hexdigest(),fps=30,duration=1))
        examples.append(dict(id=name,assets=assets))
    data=dict(schema_version=1,examples=examples,publication_authorized=False)
    (tmp_path/'selection.json').write_text(json.dumps(data))
    manifest={'homepage_media':{'selection':'selection.json'},'examples':[]}
    m=tmp_path/'manifest.yaml';m.write_text('homepage_media:\n  selection: selection.json\nexamples: []\n')
    return data,manifest,m,products


def test_selected_multicodec_stage_and_inventory(tmp_path,monkeypatch):
    data,manifest,m,products=snapshot(tmp_path,monkeypatch)
    target=tmp_path/'stage';stage.stage_homepage_products(target,m,products,require_video=True)
    for asset in selected.assets_for(data):selected.verify_asset(target/'profiles/homepage',asset)
    urls,missing,expected=availability.combined_video_assets(manifest,target)
    assert len(urls)==5 and expected==5 and not missing
    assert any(url.endswith('.av1.mp4') for url in urls)


def test_tampered_selected_asset_is_rejected(tmp_path,monkeypatch):
    data,_,m,products=snapshot(tmp_path,monkeypatch)
    asset=selected.assets_for(data)[0];(products/asset['path']).write_bytes(b'corruption')
    with pytest.raises(ValueError,match='mismatch'):stage.stage_homepage_products(tmp_path/'stage',m,products,require_video=True)


def test_hydration_preserves_reviewed_bytes_without_encoder(tmp_path,monkeypatch):
    data,manifest,m,products=snapshot(tmp_path,monkeypatch)
    args=argparse.Namespace(capture_profile='homepage',manifest=m,jobs='1',capture_jobs='1',staging_root=tmp_path/'stage/profiles/homepage',availability_manifest=tmp_path/'stage/local-video-assets.json',watched_sentinel=tmp_path/'ready')
    with mock.patch.object(hydration,'comparison_args'),mock.patch.object(hydration.comparison_media,'selected_previews',side_effect=AssertionError('must not encode')):
        result=hydration.hydrate(args,mock.Mock())
    assert result.generated==0 and result.expected==5 and len(result.available)==5
    for asset in selected.assets_for(data):selected.verify_asset(args.staging_root,asset)


def test_selection_source_set_verified_before_hydration_writes(tmp_path,monkeypatch):
    data,manifest,m,products=snapshot(tmp_path,monkeypatch)
    last=selected.assets_for(data)[-1];(products/last['path']).unlink()
    args=argparse.Namespace(capture_profile='homepage',manifest=m,jobs='1',capture_jobs='1',staging_root=tmp_path/'served',availability_manifest=None,watched_sentinel=tmp_path/'ready')
    with mock.patch.object(hydration,'comparison_args'),pytest.raises(ValueError,match='Missing selected'):
        hydration.hydrate(args,mock.Mock())
    assert not args.staging_root.exists()


def test_hermetic_selection_uses_placeholders_without_binary_cache(tmp_path,monkeypatch):
    data,_,m,products=snapshot(tmp_path,monkeypatch)
    stage.stage_homepage_products(tmp_path/'hermetic',m,None,hermetic=True)
    assert len(list((tmp_path/'hermetic').rglob('*.webp')))==4
    assert not list((tmp_path/'hermetic').rglob('*.mp4'))


@pytest.mark.parametrize('path',['/private/file.mp4','../file.mp4','showcases/../../file.mp4','showcases/wrong.mp4'])
def test_unsafe_selection_asset_rejected(tmp_path,monkeypatch,path):
    data,_,_,_=snapshot(tmp_path,monkeypatch)
    data['examples'][0]['assets'][0]['path']=path
    with pytest.raises(ValueError):selected.validate_selection(data)


def test_package_is_deterministic_and_publication_gate_stays_closed(tmp_path,monkeypatch):
    _,_,m,products=snapshot(tmp_path,monkeypatch)
    first,second=tmp_path/'first.tar.gz',tmp_path/'second.tar.gz'
    for path in (first,second):
        monkeypatch.setattr(sys,'argv',['homepage_media.py','--manifest',str(m),'--products',str(products),'--package',str(path)])
        assert selected.main()==0
    assert first.read_bytes()==second.read_bytes()
    monkeypatch.setattr(sys,'argv',['homepage_media.py','--manifest',str(m),'--products',str(products),'--publication'])
    assert selected.main()==1
