"""Explicit candidate encoders use canonical frames and bounded, recorded settings."""
import argparse
import copy
from dataclasses import replace
from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import compare_gallery_media as media
import gallery_frames
import gallery_media


def av1_settings():
    return dict(encoder_version='4.3.0', preset=3, tune=4, crf=56, fps=30,
                video_budget_bytes=5_000_000, temporal_filtering=False, film_grain=0)


def entry():
    return {'id': 'showcases_point_cloud', 'category': 'showcase', 'media': {
        'preview': {'kind': 'animated-webp', 'frames': 360, 'fps': 60,
                    'card': {'preferred': 'video-mp4', 'sample_step': 1, 'fps': 60}},
        'profiles': {'homepage': {'frames': 576, 'fps': 60, 'presentation': {'kind': 'sway'},
            'card': {'av1': av1_settings(), 'mp4_bitrate_kbps': 3000, 'mp4_preset': 'veryslow',
                     'mp4_fps': 30, 'mp4_two_pass': True, 'video_budget_bytes': 4_000_000}}}}}


def test_codec_profile_derives_output_sampling_without_changing_capture():
    effective = gallery_media.resolve_media_entry(entry(), 'homepage')
    preview = effective['media']['preview']
    assert preview['fps'] == 60
    assert (preview['card']['fps'], preview['card']['sample_step']) == (30, 2)
    args = argparse.Namespace(profile='homepage', preferred_kind='animated-webp', step=2,
        webp_quality=40, mp4_crf=32, webm_crf=38, fps=0, no_manifest_card=False)
    descriptor = type('Preview', (), {'lane': 'showcases', 'id': 'showcases_point_cloud', 'fps': 60})()
    profile = media.profile_for(descriptor, args, {('showcases', descriptor.id): entry()})
    assert (profile.fps, profile.step, profile.mp4_bitrate_kbps, profile.mp4_two_pass) == (30, 2, 3000, True)
    assert media.generated_variant_kinds(profile, False) == {'mp4-card', 'av1-card'}
    first = gallery_frames.capture_manifest_hash(effective)
    changed = entry()
    changed['media']['profiles']['homepage']['card']['av1']['crf'] = 52
    assert first == gallery_frames.capture_manifest_hash(gallery_media.resolve_media_entry(changed, 'homepage'))


@pytest.mark.parametrize('key,value', [('encoder_version', '4.2.0'), ('preset', float('nan')),
    ('crf', 71), ('tune', True), ('fps', 29), ('video_budget_bytes', 0),
    ('temporal_filtering', True), ('film_grain', 1), ('unknown', 1)])
def test_invalid_av1_settings_fail_before_encoding(key, value):
    value_entry = entry()
    value_entry['media']['profiles']['homepage']['card']['av1'][key] = value
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(value_entry, 'homepage')


@pytest.mark.parametrize('key,value', [('mp4_bitrate_kbps', float('inf')), ('mp4_fps', 29),
    ('mp4_two_pass', 1), ('mp4_preset', 'bad')])
def test_invalid_h264_target_settings_fail_before_encoding(key, value):
    value_entry = entry()
    value_entry['media']['profiles']['homepage']['card'][key] = value
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(value_entry, 'homepage')


def configure_encoder(tmp_path, monkeypatch, version='4.3.0'):
    encoder = tmp_path / 'SvtAv1EncApp'
    encoder.write_bytes(b'encoder binary')
    monkeypatch.setenv('SVT_AV1_ENC', str(encoder))
    monkeypatch.setattr(media.subprocess, 'run', lambda *args, **kwargs:
        type('Result', (), {'stdout': f'SVT-AV1 v{version} (release)', 'stderr': ''})())
    monkeypatch.setattr(media, 'require_tool', lambda name: name)
    return encoder


def test_av1_commands_match_selected_10bit_benchmark(tmp_path, monkeypatch):
    encoder = configure_encoder(tmp_path, monkeypatch)
    output = tmp_path / 'candidate.av1.mp4'
    commands = []
    def run(command):
        commands.append(command)
        if command[-1] == str(output):
            output.write_bytes(b'encoded av1')
    monkeypatch.setattr(media, 'run', run)
    provenance = media.encode_av1('source/frame_%04d.png', output, 60, 576, av1_settings(), tmp_path)
    assert commands[0] == ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-framerate', '60',
        '-i', 'source/frame_%04d.png', '-vf', 'fps=30,format=yuv420p10le', '-strict', '-1',
        '-f', 'yuv4mpegpipe', str(tmp_path / 'av1-source.y4m')]
    assert commands[1] == [str(encoder), '--passes', '1', '--rc', '0', '--crf', '56', '--preset', '3',
        '--lp', '3', '--tune', '4', '--enable-tf', '0', '--film-grain', '0', '--fps-num', '30',
        '--fps-denom', '1', '--keyint', '288', '--input-depth', '10', '--progress', '0',
        '-i', str(tmp_path / 'av1-source.y4m'), '-b', str(tmp_path / 'av1.ivf')]
    assert commands[2][-5:] == ['-c:v', 'copy', '-movflags', '+faststart', str(output)]
    assert provenance['sha256'] == media.file_sha256(output)
    assert provenance['binary_sha256'] == media.file_sha256(encoder)


def test_encoder_version_and_binary_identity_are_enforced(tmp_path, monkeypatch):
    encoder = configure_encoder(tmp_path, monkeypatch)
    first = media.svt_av1_identity(av1_settings())
    encoder.write_bytes(b'different binary')
    assert first != media.svt_av1_identity(av1_settings())
    configure_encoder(tmp_path, monkeypatch, version='4.2.0')
    with pytest.raises(RuntimeError, match='requires SVT-AV1 version 4.3.0'):
        media.svt_av1_identity(av1_settings())


def test_two_pass_h264_uses_exact_native_frame_filter_and_shared_pass_log(tmp_path, monkeypatch):
    monkeypatch.setattr(media, 'require_tool', lambda name: name)
    output = tmp_path / 'fallback.mp4'
    commands = []
    def run(command):
        commands.append(command)
        if command[-1] == str(output):
            output.write_bytes(b'h264')
    monkeypatch.setattr(media, 'run', run)
    profile = media.EncodingProfile('video-mp4', 2, 40, 24, 38, fps=30,
        mp4_bitrate_kbps=3000, mp4_preset='veryslow', mp4_two_pass=True, video_budget_bytes=4_000_000)
    media.encode_bitrate_mp4('source/frame_%04d.png', output, 60, 576, profile, tmp_path)
    for command in commands:
        assert command[command.index('-vf') + 1] == 'fps=30,format=yuv420p'
        assert command[command.index('-threads') + 1] == '8'
        assert command[command.index('-profile:v') + 1] == 'high'
        assert command[command.index('-level:v') + 1] == '3.1'
        assert command[command.index('-refs') + 1] == '4'
        assert command[command.index('-bf') + 1] == '3'
        assert command[command.index('-g') + 1] == '288'
        assert command[command.index('-b:v') + 1] == '3000k'
        assert command[command.index('-passlogfile') + 1] == str(tmp_path / 'h264-pass')
    assert commands[0][-4:] == ['1', '-f', 'null', media.os.devnull]
    assert commands[1][-5:] == ['-pass', '2', '-movflags', '+faststart', str(output)]


def test_candidate_report_site_products_and_cache_include_both_codecs(tmp_path, monkeypatch):
    encoder = configure_encoder(tmp_path, monkeypatch)
    preview = type('Preview', (), {'id': 'showcases_point_cloud', 'lane': 'showcases',
        'title': 'Point cloud', 'frames': 4, 'fps': 60, 'size': '1280x720'})()
    frames = tmp_path / 'frames'
    frames.mkdir()
    for index in range(4):
        (frames / f'frame_{index:04d}.png').write_bytes(b'frame')
    sequence = gallery_frames.FrameSequence(preview, frames, 'inputs', 'frames', False, 'current')
    profile = media.EncodingProfile('video-mp4', 2, 40, 24, 38, fps=30, av1=av1_settings(),
        mp4_bitrate_kbps=3000, mp4_preset='veryslow', mp4_two_pass=True, video_budget_bytes=4_000_000)
    args = argparse.Namespace(input_dir=tmp_path / 'webp', output_dir=tmp_path / 'cards', force=False,
        dry_run=False, webm=False)
    def av1_encode(pattern, output, *unused):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(b'av1')
        return {'sha256': media.file_sha256(output), 'settings': av1_settings()}
    def mp4_encode(pattern, output, *unused):
        output.write_bytes(b'h264')
    monkeypatch.setattr(media, 'encode_av1', av1_encode)
    monkeypatch.setattr(media, 'encode_bitrate_mp4', mp4_encode)
    monkeypatch.setattr(media, 'encode_poster', lambda source, target, quality: target.write_bytes(b'poster'))
    result = media.compare_preview(preview, args, sequence, profile)
    assert (result.encoded_fps, result.encoded_frames, result.encoded_duration) == (30, 2, 4 / 60)
    assert media.variant_for(result, 'av1-card').budget_bytes == 5_000_000
    assert media.variant_for(result, 'mp4-card').budget_bytes == 4_000_000
    assert {variant.kind for variant in media.budgeted_variants(result)} == {'poster', 'mp4-card', 'av1-card'}
    assert result.codec_provenance['av1']['settings']['tune'] == 4
    cache = tmp_path / 'cache'
    key = media.comparison_input_hash(preview, sequence, profile, args, 'tools')
    encoder.write_bytes(b'changed encoder')
    assert media.comparison_input_hash(preview, sequence, profile, args, 'tools') != key
    media.write_comparison_cache(preview, cache, key, result)
    assert media.current_comparison_cache(preview, cache, key) == result
    manifest = tmp_path / 'manifest.yaml'
    import yaml
    manifest.write_text(yaml.safe_dump({'examples': [entry()]}))
    site = tmp_path / 'site'
    assert media.write_site_assets(site, [result], manifest) == 3
    assert (site / 'showcases/showcases_point_cloud.av1.mp4').read_bytes() == b'av1'
    assert (site / 'showcases/showcases_point_cloud.mp4').read_bytes() == b'h264'
    Path(media.variant_for(result, 'av1-card').path).write_bytes(b'bad')
    assert media.current_comparison_cache(preview, cache, key) is None


def test_av1_budget_failure_keeps_settings_and_never_falls_back(tmp_path, monkeypatch):
    configure_encoder(tmp_path, monkeypatch)
    output = tmp_path / 'av1.mp4'
    calls = []
    def run(command):
        calls.append(command)
        if command[-1] == str(output):
            output.write_bytes(b'oversized')
    monkeypatch.setattr(media, 'run', run)
    settings = {**av1_settings(), 'video_budget_bytes': 1}
    with pytest.raises(RuntimeError, match='settings were preserved'):
        media.encode_av1('frames/frame_%04d.png', output, 60, 576, settings, tmp_path)
    assert len(calls) == 3
    assert calls[1][calls[1].index('--crf') + 1] == '56'


@pytest.mark.parametrize('crf', [1, 46.25, 46.5, 70])
def test_av1_accepts_quarter_step_quality(crf):
    value = entry()
    value['media']['profiles']['homepage']['card']['av1']['crf'] = crf
    resolved = gallery_media.resolve_media_entry(value, 'homepage')
    assert resolved['media']['preview']['card']['av1']['crf'] == crf


@pytest.mark.parametrize('crf', [True, 0, 46.1, float('nan'), float('inf')])
def test_av1_rejects_invalid_fractional_quality(crf):
    value = entry()
    value['media']['profiles']['homepage']['card']['av1']['crf'] = crf
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(value, 'homepage')


def test_quality_fallback_uses_shared_frames_and_preserves_quality_budget(tmp_path, monkeypatch):
    source = tmp_path / 'av1-source.y4m'
    source.write_bytes(b'uncompressed source')
    output = tmp_path / 'fallback.mp4'
    commands = []
    def run(command):
        commands.append(command)
        output.write_bytes(b'video')
    monkeypatch.setattr(media, 'run', run)
    monkeypatch.setattr(media, 'require_tool', lambda name: name)
    profile = media.EncodingProfile('video-mp4', 2, 85, 24, 38, fps=30, mp4_preset='veryslow')
    media.encode_quality_fallback(source, output, 60, 864, profile)
    assert commands[0] == ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(source),
        '-an', '-vf', 'format=yuv420p', '-threads', '8', '-c:v', 'libx264', '-preset', 'veryslow',
        '-profile:v', 'high', '-level:v', '3.1', '-refs', '4', '-bf', '3', '-crf', '24', '-g', '432',
        '-movflags', '+faststart', str(output)]
    with pytest.raises(RuntimeError, match='quality was preserved'):
        media.encode_quality_fallback(source, output, 60, 864, replace(profile, video_budget_bytes=1))
