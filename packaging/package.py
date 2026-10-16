"""Build local candidate packages after a successful Release build and CTest run.
Requires Python 3 and Inno Setup 6 on the packaging machine only.
"""
import argparse
import hashlib
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = '1.0.0-rc.1'
PREFIX = f'SleepwalkRadio-{VERSION}'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archive(destination, entries):
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for source, name in sorted(entries, key=lambda item: item[1]):
            z.write(source, name)
    with zipfile.ZipFile(destination) as z:
        if z.testzip() is not None:
            raise RuntimeError(f'Invalid archive: {destination}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--iscc', default=r'C:\Program Files (x86)\Inno Setup 6\ISCC.exe')
    parser.add_argument('--reuse-installer', action='store_true',
                        help='Refresh documentation/source ZIPs without rebuilding the tested installer')
    args = parser.parse_args()
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    if f'"{VERSION}"' not in cmake:
        raise RuntimeError('Package and CMake candidate versions disagree')
    bundle = ROOT / 'build/SleepwalkRadio_artefacts/Release/VST3/SleepwalkRadio.vst3'
    binary = bundle / 'Contents/x86_64-win/SleepwalkRadio.vst3'
    manifest = (bundle / 'Contents/Resources/moduleinfo.json').read_text()
    for expected in ('"Version": "1.0.0"', 'ABCDEF019182FAEB5377726453777231',
                     'ABCDEF011234ABCD5377726453777231'):
        if expected not in manifest:
            raise RuntimeError(f'Incorrect VST3 identity/version: {expected}')
    if binary.stat().st_size < 100000:
        raise RuntimeError('Missing/truncated plugin')
    juce = ROOT / '.deps/JUCE'
    if not (juce / 'CMakeLists.txt').exists():
        raise RuntimeError('Pinned JUCE source required for matching source package')
    if (juce / '.git').exists():
        revision = subprocess.check_output(['git', '-C', str(juce), 'rev-parse', 'HEAD'], text=True).strip()
        if revision != '29396c22c93392d6738e021b83196283d6e4d850':
            raise RuntimeError('Unexpected JUCE revision')
    dist = ROOT / 'dist'
    stage = ROOT / 'packaging/stage'
    # Only this script's fixed staging directory is replaced; never installed files.
    if stage.exists():
        if stage.resolve() != ROOT / 'packaging/stage':
            raise RuntimeError('Unsafe stage path')
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    dist.mkdir(exist_ok=True)
    shutil.copytree(bundle, stage / 'SleepwalkRadio.vst3')
    for name in ('LICENSE', 'THIRD_PARTY_NOTICES.md', 'RELEASE_NOTES.md'):
        shutil.copy2(ROOT / name, stage / name)
    shutil.copy2(ROOT / 'packaging/INSTALL.txt', stage / 'INSTALL.txt')
    shutil.copytree(ROOT / 'licenses', stage / 'licenses')
    archive(dist / f'{PREFIX}-Windows-x64-VST3.zip',
            [(p, p.relative_to(stage).as_posix()) for p in stage.rglob('*') if p.is_file()])
    # Include project source and full pinned dependency source; omit all .git metadata/builds.
    sources = []
    for folder in ('Source', 'Tests', 'docs', 'licenses', 'packaging'):
        for p in (ROOT / folder).rglob('*'):
            if p.is_file() and 'stage' not in p.relative_to(ROOT).parts and '__pycache__' not in p.parts:
                sources.append((p, p.relative_to(ROOT).as_posix()))
    for name in ('CMakeLists.txt', '.gitignore', 'README.md', 'PROJECT_STATUS.md',
                 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'CHANGELOG.md', 'RELEASE_NOTES.md'):
        sources.append((ROOT / name, name))
    for p in juce.rglob('*'):
        relative = p.relative_to(juce)
        if p.is_file() and '.git' not in relative.parts:
            sources.append((p, '.deps/JUCE/' + relative.as_posix()))
    archive(dist / f'{PREFIX}-Source.zip', sources)
    if not args.reuse_installer:
        subprocess.run([args.iscc, '/Qp', str(ROOT / 'packaging/SleepwalkRadio.iss')], check=True)
    assets = [dist / f'{PREFIX}-Windows-x64-Setup.exe',
              dist / f'{PREFIX}-Windows-x64-VST3.zip', dist / f'{PREFIX}-Source.zip']
    (dist / 'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in assets), encoding='ascii')
    with zipfile.ZipFile(assets[1]) as z:
        packed = z.read('SleepwalkRadio.vst3/Contents/x86_64-win/SleepwalkRadio.vst3')
        if hashlib.sha256(packed).hexdigest() != sha(binary):
            raise RuntimeError('Packaged plugin does not match build')
    for p in assets:
        print(f'{p.name}: {p.stat().st_size} bytes; SHA256 {sha(p)}')
    print('ZIP integrity and packaged binary identity passed. Installer execution is separate QA.')


if __name__ == '__main__':
    main()
