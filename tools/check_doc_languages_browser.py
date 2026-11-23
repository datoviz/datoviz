#!/usr/bin/env python3
"""Exercise page-wide Python/C selection in every mixed page of a built documentation site.

Requires Playwright and Chromium. All requests stay inside an in-memory HTTPS fixture; this check
needs no HTTP server, graphics device, media downloads, fonts, or external diagram services.
"""

from __future__ import annotations

import argparse
import asyncio
import html
import json
import mimetypes
import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ORIGIN = 'https://docs-fixture.invalid'
PREFIX = '/datoviz-docs/'
KEY = 'datoviz.docs.language.v1'
LANGUAGES = ('Python', 'C')

# Inspect rendered code containers as well as code itself: Pygments puts language-* on the
# surrounding highlight div. Inline code and non-host languages never enter the visibility check.
STATE = r"""() => {
 const article=document.querySelector('article');
 if(!article)return {error:'Missing article'};
 const aliases={python:'Python',py:'Python',python3:'Python',pycon:'Python',c:'C',cpp:'C','c++':'C',h:'C',hpp:'C'};
 const visible=e=>e.getClientRects().length>0&&getComputedStyle(e).visibility!=='hidden';
 const languages=code=>{
   const values=[];
   for(let p=code;p&&p!==article;p=p.parentElement)
     for(const cls of p.classList)if(cls.startsWith('language-'))values.push(cls.slice(9));
   return [...new Set(values)];
 };
 const codeState=code=>{
   const raw=languages(code),families=[...new Set(raw.map(n=>aliases[n]).filter(Boolean))];
   return {family:families.length===1?families[0]:null,languages:raw,visible:visible(code),
     sample:code.textContent.trim().slice(0,140)};
 };
 const sets=[];
 for(const set of article.querySelectorAll('.tabbed-set')){
   const labels=[...set.querySelectorAll(':scope > .tabbed-labels > label')];
   const names=labels.map(l=>l.textContent.trim());
   if(!names.includes('Python')||!names.includes('C'))continue;
   set.dataset.languageAuditSet=String(sets.length);
   const blocks=[...set.querySelectorAll(':scope > .tabbed-content > .tabbed-block')];
   const tabs=labels.map((label,index)=>{
     const input=document.getElementById(label.htmlFor);
     const codes=[...(blocks[index]?.querySelectorAll('pre code')||[])].map(codeState);
     return {name:names[index],input:label.htmlFor,checked:!!input?.checked,labelVisible:visible(label),
       families:[...new Set(codes.map(c=>c.family).filter(Boolean))],
       languages:[...new Set(codes.flatMap(c=>c.languages))],untypedCode:codes.filter(c=>!c.languages.length).length};
   });
   sets.push({index:sets.length,names,selected:tabs.filter(t=>t.checked).map(t=>t.name),tabs});
 }
 let saved=null;try{saved=localStorage.getItem('datoviz.docs.language.v1')}catch(_){}
 return {ready:article.dataset.dvzLanguagesReady||null,sets,codes:[...article.querySelectorAll('pre code')].map(codeState),saved,
   details:[...article.querySelectorAll('details')].filter(d=>d.querySelector('pre code')).map(d=>({open:d.open})),
   overflow:document.documentElement.scrollWidth>innerWidth+1};
}"""

READY = r"""expected => {
 const article=document.querySelector('article');
 if(!article)return false;
 const sets=[...article.querySelectorAll('.tabbed-set')].filter(s=>{
   const names=[...s.querySelectorAll(':scope > .tabbed-labels > label')].map(l=>l.textContent.trim());
   return names.includes('Python')&&names.includes('C');
 });
 return sets.length>0&&sets.every(s=>[...s.querySelectorAll(':scope > .tabbed-labels > label')]
   .filter(l=>l.textContent.trim()===expected).every(l=>document.getElementById(l.htmlFor)?.checked));
}"""


def discover(site: Path) -> list[str]:
    """Discover actual built pages, including generated source inclusions."""
    pages = []
    for path in sorted(site.rglob('*.html')):
        source = path.read_text(errors='replace')
        article = re.search(r'<article\b[^>]*>(.*?)</article>', source, re.S)
        if not article:
            continue
        labels = {
            html.unescape(re.sub(r'<[^>]*>', '', text)).strip()
            for text in re.findall(r'<label\b[^>]*>(.*?)</label>', article[1], re.S)
        }
        if set(LANGUAGES) <= labels:
            pages.append(path.relative_to(site).as_posix())
    return pages


def page_url(path: str) -> str:
    """Use normal directory URLs so relative site links keep their published behavior."""
    return ORIGIN + PREFIX + (path[:-10] if path.endswith('index.html') else path)


async def fixture(context, site: Path):
    """Serve only built local files; abort unrelated origins and embedded applications."""

    async def route(request):
        parsed = urlsplit(request.request.url)
        if f'{parsed.scheme}://{parsed.netloc}' != ORIGIN or not parsed.path.startswith(PREFIX):
            await request.abort()
            return
        if request.request.resource_type == 'document' and request.request.frame.parent_frame:
            await request.abort()
            return
        path = (site / unquote(parsed.path[len(PREFIX) :])).resolve()
        if not path.is_relative_to(site):
            await request.fulfill(status=403, body='Outside documentation fixture')
            return
        if path.is_dir():
            path /= 'index.html'
        if not path.is_file():
            await request.fulfill(status=404, body='Missing documentation fixture file')
            return
        await request.fulfill(
            path=str(path),
            content_type=mimetypes.guess_type(path)[0] or 'application/octet-stream',
        )

    await context.route('**/*', route)


async def snapshot(page, expected: str, *, exceptions: dict[int, str] | None = None) -> dict:
    """Reject checked-radio disagreement and any visible code in the opposite host language."""
    state = await page.evaluate(STATE)
    assert 'error' not in state, state
    assert state['sets'], 'No Python/C tab sets found in mixed page'
    for group in state['sets']:
        assert all(group['names'].count(name) == 1 for name in LANGUAGES), group
        wanted = (exceptions or {}).get(group['index'], expected)
        assert group['selected'] == [wanted], {'expected': wanted, 'set': group}
        assert all(not tab['untypedCode'] for tab in group['tabs'] if tab['name'] in LANGUAGES), {
            'missing_host_language_classes': group
        }
    wrong = [c for c in state['codes'] if c['visible'] and c['family'] and c['family'] != expected]
    assert not wrong, {'expected': expected, 'opposite_visible_code': wrong}
    return state


async def settle(page, expected: str):
    """Wait for the real documentation script, without waiting on network services."""
    try:
        await page.wait_for_function(READY, arg=expected, timeout=5000)
    except Exception as error:
        state = await page.evaluate(STATE)
        raise AssertionError(
            {
                'page': page.url,
                'expected_language': expected,
                'ready': state.get('ready'),
                'selected': [
                    {'names': g['names'], 'selected': g['selected']} for g in state.get('sets', [])
                ],
                'saved_preference': state.get('saved'),
            }
        ) from error
    await page.wait_for_timeout(35)


async def load(page, path: str, expected: str):
    response = await page.goto(page_url(path), wait_until='domcontentloaded')
    assert response and response.status == 200, {
        'path': path,
        'status': response.status if response else None,
    }
    await settle(page, expected)


async def choose(page, language: str):
    """Click a genuine visible host-language label, opening one source disclosure if needed."""
    state = await page.evaluate(STATE)
    candidates = [
        tab
        for group in state.get('sets', [])
        for tab in group['tabs']
        if tab['name'] == language and tab['labelVisible']
    ]
    if not candidates:
        await page.evaluate("""() => {
          const set=document.querySelector('[data-language-audit-set]');
          for(let p=set?.parentElement;p;p=p.parentElement)if(p.tagName==='DETAILS')p.open=true;
        }""")
        await page.wait_for_timeout(30)
        state = await page.evaluate(STATE)
        candidates = [
            tab
            for group in state.get('sets', [])
            for tab in group['tabs']
            if tab['name'] == language and tab['labelVisible']
        ]
    assert candidates, {'missing_clickable_language': language, 'state': state}
    await page.locator('label[for="' + candidates[0]['input'] + '"]').click()
    await settle(page, language)


async def disclosures(page, opened: bool) -> int:
    count = await page.evaluate(
        """open => {
      const ds=[...document.querySelectorAll('article details')].filter(d=>d.querySelector('pre code'));
      ds.forEach(d=>d.open=open);return ds.length;
    }""",
        opened,
    )
    await page.wait_for_timeout(40)
    return count


def summarize(state: dict) -> dict:
    """Keep useful coverage without copying complete generated source listings into reports."""
    return {
        'tabsets': state['sets'],
        'host_code_blocks': {
            language: sum(c['family'] == language for c in state['codes'])
            for language in LANGUAGES
        },
        'visible_host_code_blocks': {
            language: sum(c['family'] == language and c['visible'] for c in state['codes'])
            for language in LANGUAGES
        },
        'code_disclosures': len(state['details']),
        'saved_preference': state['saved'],
        'page_overflow': state['overflow'],
    }


async def audit_page(page, path: str, expected: str, final: str, *, require_storage=True) -> dict:
    await load(page, path, expected)
    initial = await snapshot(page, expected)
    checks = []
    for language in LANGUAGES:
        await disclosures(page, False)
        await choose(page, language)
        closed = await snapshot(page, language)
        await disclosures(page, True)
        opened = await snapshot(page, language)
        checks.append(
            {'language': language, 'closed': summarize(closed), 'opened': summarize(opened)}
        )
    await choose(page, final)
    final_state = await snapshot(page, final)
    if require_storage:
        assert final_state['saved'], {
            'page': page.url,
            'error': 'Language preference was not persisted',
        }
    return {
        'path': path,
        'initial_preference': expected,
        'initial': summarize(initial),
        'choices': checks,
        'final_preference': final,
        'passed': True,
    }


async def specialized(
    context, pages: list[str], screenshots: Path | None, *, results=None, auxiliary_paths=None
) -> list[dict]:
    """Exercise navigation, refresh, history, keyboard input, auxiliaries and unrelated tabs."""
    if results is None:
        results = []
    page = await context.new_page()
    axes = 'how-to/axes/index.html'
    start = 'how-to/use-colormaps/index.html'
    target = 'how-to/update-visual-data/index.html'
    for required in (
        axes,
        start,
        target,
        'gpu-graphics/01-setup/index.html',
        'reference/c-api/scene/index.html',
    ):
        assert required in pages or required.startswith(('gpu-graphics/', 'reference/c-api/')), (
            required
        )
    await page.goto(page_url(start) + '#__tabbed_1_1', wait_until='domcontentloaded')
    await choose(page, 'Python')
    await page.locator('article a[href*="update-visual-data"]').first.click()
    await page.wait_for_url(page_url(target))
    await settle(page, 'Python')
    await snapshot(page, 'Python')
    await choose(page, 'C')
    await page.go_back(wait_until='domcontentloaded')
    await settle(page, 'C')
    await snapshot(page, 'C')
    await page.reload(wait_until='domcontentloaded')
    await settle(page, 'C')
    await snapshot(page, 'C')
    other = await context.new_page()
    await load(other, axes, 'C')
    await snapshot(other, 'C')
    # A preference changed in another tab must also reach an already open document.
    await choose(other, 'Python')
    await settle(page, 'Python')
    await snapshot(page, 'Python')
    await other.close()
    results.append(
        {'case': 'real-link navigation, back, refresh, new page and storage event', 'passed': True}
    )

    await load(page, axes, 'Python')
    await choose(page, 'C')
    state = await page.evaluate(STATE)
    checked = next(t['input'] for t in state['sets'][0]['tabs'] if t['checked'])
    assert await page.evaluate('document.activeElement.id') == checked, {
        'case': 'Pointer establishes radio focus',
        'page': page.url,
        'expected_input': checked,
    }
    await page.keyboard.press('ArrowLeft')
    await settle(page, 'Python')
    await snapshot(page, 'Python')
    results.append(
        {'case': 'keyboard radio selection synchronizes all host-language sets', 'passed': True}
    )

    await page.goto(page_url('gpu-graphics/01-setup/index.html'), wait_until='domcontentloaded')
    label = page.locator('article label').filter(has_text=re.compile(r'^macOS$')).first
    await label.click()
    os_state = await page.evaluate("""()=>[...document.querySelectorAll('article .tabbed-set')].map(s=>{
       const labels=[...s.querySelectorAll(':scope > .tabbed-labels > label')];
       if(!['Linux','macOS','Windows'].every(n=>labels.some(l=>l.textContent.trim()===n)))return null;
       return {visible:s.getClientRects().length>0&&!s.closest('details:not([open])'),selected:labels.find(l=>document.getElementById(l.htmlFor)?.checked)?.textContent.trim()};
    }).filter(Boolean)""")
    assert (
        os_state
        and any(n['visible'] for n in os_state)
        and all(n['selected'] == 'macOS' for n in os_state if n['visible'])
    ), {
        'case': 'Initial OS selection',
        'page': page.url,
        'selected': os_state,
    }
    await load(page, axes, 'Python')
    await choose(page, 'C')
    await choose(page, 'Python')
    await page.goto(page_url('gpu-graphics/01-setup/index.html'), wait_until='domcontentloaded')
    pure = await page.evaluate(STATE)
    assert not pure['sets'] and any(c['family'] == 'C' and c['visible'] for c in pure['codes']), (
        pure
    )
    # Native OS linking updates hidden groups when their disclosure opens. This is the
    # course's existing contract, distinct from host-language synchronization of every radio.
    await disclosures(page, True)
    await page.wait_for_function(
        """()=>[...document.querySelectorAll('article .tabbed-set')].every(s=>{
      const labels=[...s.querySelectorAll(':scope > .tabbed-labels > label')];
      if(!['Linux','macOS','Windows'].every(n=>labels.some(l=>l.textContent.trim()===n))||!s.getClientRects().length||s.closest('details:not([open])'))return true;
      return labels.find(l=>document.getElementById(l.htmlFor)?.checked)?.textContent.trim()==='macOS';
    })""",
        timeout=5000,
    )
    selected = await page.evaluate("""()=>[...document.querySelectorAll('article .tabbed-set')]
      .filter(s=>s.getClientRects().length>0&&!s.closest('details:not([open])')).map(s=>{
        const labels=[...s.querySelectorAll(':scope > .tabbed-labels > label')];
        if(!['Linux','macOS','Windows'].every(n=>labels.some(l=>l.textContent.trim()===n)))return null;
        return labels.find(l=>document.getElementById(l.htmlFor)?.checked)?.textContent.trim();
      }).filter(Boolean)""")
    assert selected and all(name == 'macOS' for name in selected), {
        'case': 'Visible OS selection after host-language changes and disclosure opening',
        'page': page.url,
        'selected': selected,
    }
    await page.goto(page_url('gpu-graphics/04-triangle/index.html'), wait_until='domcontentloaded')
    shader_page = await page.evaluate(STATE)
    assert (
        not shader_page['sets']
        and any(c['family'] == 'C' and c['visible'] for c in shader_page['codes'])
        and any('glsl' in c['languages'] and c['visible'] for c in shader_page['codes'])
    ), {'case': 'C-only course with common GLSL', 'page': page.url, 'state': shader_page}
    await page.goto(page_url('reference/c-api/scene/index.html'), wait_until='domcontentloaded')
    pure = await page.evaluate(STATE)
    assert not pure['sets'] and any(c['family'] == 'C' and c['visible'] for c in pure['codes']), (
        pure
    )
    results.append(
        {
            'case': 'OS selection survives host changes; C-only course, common GLSL and C reference remain visible',
            'passed': True,
        }
    )

    # Extra source panes may contain C models, Python preparation tools or shaders. Their labels
    # are intentionally distinct from the primary C/Python choice and must remain usable.
    auxiliary = []
    for path in pages if auxiliary_paths is None else auxiliary_paths:
        if not path.startswith('examples/'):
            continue
        await load(page, path, 'Python')
        state = await page.evaluate(STATE)
        extras = [
            (group, tab)
            for group in state['sets']
            for tab in group['tabs']
            if tab['name'] not in LANGUAGES
        ]
        if not extras:
            continue
        for group, tab in extras:
            if not tab['labelVisible']:
                await disclosures(page, True)
            await page.locator('label[for="' + tab['input'] + '"]').click()
            family = tab['families'][0] if len(tab['families']) == 1 else None
            expected = family or 'Python'
            await page.wait_for_timeout(50)
            await snapshot(page, expected, exceptions={group['index']: tab['name']})
            await choose(page, 'Python')
            auxiliary.append(
                {'path': path, 'label': tab['name'], 'languages': tab['languages'], 'passed': True}
            )
    results.append(
        {
            'case': 'auxiliary source panes preserve their own selection and synchronize host family',
            'panes': auxiliary,
            'passed': True,
        }
    )
    if screenshots:
        await load(page, axes, 'Python')
        await page.screenshot(path=str(screenshots / 'axes-desktop-python.png'), full_page=True)
        await choose(page, 'C')
        await page.screenshot(path=str(screenshots / 'axes-desktop-c.png'), full_page=True)
    await page.close()
    return results


async def run(args) -> dict:
    try:
        from playwright.async_api import async_playwright
    except ImportError as error:
        raise RuntimeError(
            'Playwright is required; install it and its Chromium browser, or run through the project browser-check recipe'
        ) from error
    site = args.site_dir.resolve()
    if not site.is_dir():
        raise ValueError(f'Built documentation directory is missing: {site}')
    pages = discover(site)
    if not pages:
        raise ValueError('No built pages containing both Python and C tab labels were found')
    screenshots = args.screenshots_dir.resolve() if args.screenshots_dir else None
    if screenshots:
        screenshots.mkdir(parents=True, exist_ok=True)
    report = {
        'schema_version': 1,
        'discovered_mixed_pages': len(pages),
        'generated_example_pages': sum(p.startswith('examples/') for p in pages),
        'desktop': [],
        'mobile': [],
        'specialized': [],
        'errors': [],
    }

    def save():
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')

    async with async_playwright() as p:
        browser = await p.chromium.launch(
            **(
                {'executable_path': str(args.browser_executable)}
                if args.browser_executable
                else {}
            )
        )
        desktop = await browser.new_context(
            viewport={'width': 1440, 'height': 1000}, reduced_motion='reduce'
        )
        await fixture(desktop, site)
        page = await desktop.new_page()
        expected = 'Python'
        for index, path in enumerate(pages):
            final = LANGUAGES[index % 2]
            try:
                row = await audit_page(page, path, expected, final)
                report['desktop'].append(row)
                expected = final
            except Exception as error:
                report['errors'].append({'phase': 'desktop', 'path': path, 'error': str(error)})
                # Recover a known starting preference without counting recovery as a pass.
                await page.evaluate(
                    '(language)=>{try{localStorage.setItem("' + KEY + '",language)}catch(_){}}',
                    expected,
                )
            save()
            if (index + 1) % 20 == 0:
                print(f'Desktop language pages: {index + 1}/{len(pages)}', flush=True)
        try:
            auxiliary_paths = [
                row['path']
                for row in report['desktop']
                if row['path'].startswith('examples/')
                and any(
                    tab['name'] not in LANGUAGES
                    for group in row['initial']['tabsets']
                    for tab in group['tabs']
                )
            ]
            await specialized(
                desktop,
                pages,
                screenshots,
                results=report['specialized'],
                auxiliary_paths=auxiliary_paths,
            )
        except Exception as error:
            report['errors'].append({'phase': 'specialized', 'error': str(error)})
        await desktop.close()

        mobile_paths = [path for path in pages if not path.startswith('examples/')]
        # Include real generated helpers and shader panes when present, plus a normal source page.
        for row in report['desktop']:
            if not row['path'].startswith('examples/'):
                continue
            tabs = [
                t
                for s in row['initial']['tabsets']
                for t in s['tabs']
                if t['name'] not in LANGUAGES
            ]
            if tabs and (
                not any(p.startswith('examples/') for p in mobile_paths)
                or any('glsl' in t['languages'] or 'Python' in t['families'] for t in tabs)
            ):
                mobile_paths.append(row['path'])
        generated = next((p for p in pages if p.startswith('examples/')), None)
        if generated and generated not in mobile_paths:
            mobile_paths.append(generated)
        mobile = await browser.new_context(
            viewport={'width': 390, 'height': 844}, reduced_motion='reduce'
        )
        await fixture(mobile, site)
        page = await mobile.new_page()
        expected = 'Python'
        for index, path in enumerate(mobile_paths):
            final = LANGUAGES[index % 2]
            try:
                report['mobile'].append(await audit_page(page, path, expected, final))
                expected = final
                if screenshots and path == 'how-to/axes/index.html':
                    for language in LANGUAGES:
                        await choose(page, language)
                        await page.screenshot(
                            path=str(screenshots / f'axes-mobile-{language.lower()}.png'),
                            full_page=True,
                        )
                    await choose(page, expected)
            except Exception as error:
                report['errors'].append({'phase': 'mobile', 'path': path, 'error': str(error)})
                await page.evaluate(
                    '(language)=>{try{localStorage.setItem("' + KEY + '",language)}catch(_){}}',
                    expected,
                )
            save()
        await mobile.close()

        blocked = await browser.new_context(
            viewport={'width': 1280, 'height': 900}, reduced_motion='reduce'
        )
        await fixture(blocked, site)
        await blocked.add_init_script(
            'Object.defineProperty(window,"localStorage",{get(){throw new DOMException("Storage blocked for audit","SecurityError")}})'
        )
        page = await blocked.new_page()
        try:
            await audit_page(page, 'how-to/axes/index.html', 'Python', 'C', require_storage=False)
            report['specialized'].append(
                {
                    'case': 'blocked storage retains page-wide synchronization and hidden-code handling',
                    'passed': True,
                }
            )
        except Exception as error:
            report['errors'].append({'phase': 'blocked-storage', 'error': str(error)})
        await blocked.close()
        await browser.close()
    report['passed'] = (
        not report['errors']
        and len(report['desktop']) == len(pages)
        and len(report['mobile']) == len(mobile_paths)
    )
    save()
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=Path('build/docs-site-check'))
    parser.add_argument(
        '--report', type=Path, default=Path('build/doc-languages/browser-report.json')
    )
    parser.add_argument(
        '--browser-executable',
        type=Path,
        help='Optional Chrome/Chromium path; otherwise use Playwright Chromium',
    )
    parser.add_argument(
        '--screenshots-dir', type=Path, help='Optional directory for desktop/mobile axes evidence'
    )
    args = parser.parse_args()
    try:
        report = asyncio.run(run(args))
    except Exception as error:
        print(f'Documentation language browser check failed: {error}')
        return 1
    print(
        json.dumps(
            {
                'mixed_pages': report['discovered_mixed_pages'],
                'desktop_pages': len(report['desktop']),
                'mobile_pages': len(report['mobile']),
                'specialized_cases': len(report['specialized']),
                'errors': report['errors'],
                'passed': report['passed'],
            },
            indent=2,
        )
    )
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
