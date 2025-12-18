import os
import subprocess
import shutil

# Construct paths relative to the script's directory
script_dir = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.join(script_dir, '../../')
OUTPUT_ROOT_PATH = os.path.join(WORKSPACE_PATH, 'release')
SOURCE_PATH = os.path.join(WORKSPACE_PATH, 'projects')
MAKEFILE_PATH = WORKSPACE_PATH

PROJECTS = {
  'receiver': ['debug', 'release'],
  'transmitter_btl': ['debug', 'release'],
  'transmitter_app': ['debug', 'release']
}
RELEASE_FILES_FOLDERS_TO_EXCLUDE = [
  '.git',
  '.github',
  '.gitmodules',
  'cicd',
  'tools/release_creator',
]

def on_rm_error(func, path, exc_info):
    import stat
    # Is the error an access error?
    if not os.access(path, os.W_OK):
      os.chmod(path, stat.S_IWUSR)
      func(path)
    else:
      raise

def remove(path):
  if os.path.exists(path):
    if os.path.isfile(path):
      os.remove(path)
    else:
      shutil.rmtree(path, onerror=on_rm_error)

def copy(src, dst):
  dst_dir = os.path.dirname(dst)
  if not os.path.exists(dst_dir):
    os.makedirs(dst_dir)
  if os.path.isdir(src):
    shutil.copytree(src, dst)
  else:
    shutil.copy(src, dst)

def get_remote_url():
  return subprocess.run('git config --get remote.origin.url', shell=True, text=True, stdout=subprocess.PIPE).stdout.strip()

def get_branch_name():
  return subprocess.run('git rev-parse --abbrev-ref HEAD', shell=True, text=True, stdout=subprocess.PIPE).stdout.strip()

def save_source_code():
  #Clone the project repository so that we can surely get a clean source code
  cmd = f'git clone {get_remote_url()} {OUTPUT_ROOT_PATH}/src -b {get_branch_name()}'
  subprocess.run(cmd, shell=True, text=True)
  for item in RELEASE_FILES_FOLDERS_TO_EXCLUDE:
    remove(f'{OUTPUT_ROOT_PATH}/src/{item}')

def build(project_name, build_type):
  subprocess.run( f'make -C {MAKEFILE_PATH} {project_name} TYPE={build_type} TARGET=clean_build', shell=True, text=True)

  ext = 's37'
  binary_name =  f'{SOURCE_PATH}/{project_name}/{project_name}_cmake/build/{build_type.capitalize()}/{project_name}.{ext}'
  output_name = f'{OUTPUT_ROOT_PATH}/bin/{project_name}_{build_type}.{ext}'
  copy(f'{binary_name}', output_name)

remove(OUTPUT_ROOT_PATH)
for project_name, build_types in PROJECTS.items():
  for build_type in build_types:
    build(project_name, build_type)
save_source_code()
