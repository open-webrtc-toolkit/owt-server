
import os
import argparse

try:
  import urllib2 as u
except:
  import urllib.request as u


URLS = {'adapter.js': "https://webrtchacks.github.io/adapter/adapter-7.0.0.js",
        'jquery-3.2.1.min.js': "https://code.jquery.com/jquery-3.2.1.min.js",
        'socket.io.js': "https://cdnjs.cloudflare.com/ajax/libs/socket.io/2.2.0/socket.io.js",
        }

DEPS_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), 'dependencies'))


def download(file_name, url):
  req = u.Request(url)
  res_data = u.urlopen(req)
  with open(os.path.join(DEPS_PATH, file_name), 'wb') as f:
    f.write(res_data.read())


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Download some files into ./dependencies')
  parser.parse_args()
  if not os.path.exists(DEPS_PATH):
    os.makedirs(DEPS_PATH)
  for name, url in URLS.items():
    download(name, url)
