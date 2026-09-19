"""Compile/test the real monitor code with link-time HID stubs; never opens hardware."""
import os
import pathlib
import shutil
import subprocess

here = pathlib.Path(__file__).resolve().parent
root = here.parents[1]
build = here / 'build'
build.mkdir(exist_ok=True)
compiler = os.environ.get('CXX') or shutil.which('g++') or shutil.which('clang++')
if not compiler:
    raise SystemExit('C++17 compiler not found. Set CXX to g++ or clang++.')
directory = root / 'Controllers/AlienwareMonitorController/AlienwareMonitorController'
includes = [root, directory, root / 'dependencies/hidapi-win/include', root / 'dependencies/json',
            root / 'RGBController', root / 'hidapi_wrapper', root / 'i2c_smbus', root / 'SPDAccessor']
flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-pthread'] + ['-I' + str(p) for p in includes]
output = build / ('alienware-monitor-tests.exe' if os.name == 'nt' else 'alienware-monitor-tests')
subprocess.run([compiler, *flags, str(here / 'test.cpp'), str(directory / 'AlienwareMonitorController.cpp'),
                str(directory / 'AlienwareMonitorProfiles.cpp'), '-o', str(output)], check=True)
subprocess.run([str(output), str(here / 'fixtures.json')], check=True)
subprocess.run([compiler, *flags, '-fsyntax-only', str(directory / 'RGBController_AlienwareMonitor.cpp'),
                str(directory / 'AlienwareMonitorControllerDetect.cpp')], check=True)
print('RGB wrapper and detector compiled against actual upstream headers: PASS')
