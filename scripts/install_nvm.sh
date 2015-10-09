#!/bin/bash
# get origin source code
# wget -c https://raw.github.com/creationix/nvm/master/install.sh

set -e

nvm_has() {
  type "$1" > /dev/null 2>&1
  return $?
}

if [ -z "$NVM_DIR" ]; then
  NVM_DIR="$HOME/.nvm"
fi

nvm_download() {
  if nvm_has "curl"; then
    curl $*
  elif nvm_has "wget"; then
    # Emulate curl with wget
    ARGS="$*"
    ARGS=${ARGS/--progress-bar /--progress=bar }
    ARGS=${ARGS/-L /}
    ARGS=${ARGS/-I /}
    ARGS=${ARGS/-s /-q }
    ARGS=${ARGS/-o /-O }
    ARGS=${ARGS/-C /-c }
    wget $ARGS
  fi
}

install_nvm_from_git() {
  if [ -z "$NVM_SOURCE" ]; then
    NVM_SOURCE="https://github.com/creationix/nvm.git"
  fi

  if [ -d "$NVM_DIR/.git" ]; then
    echo "=> nvm is already installed in $NVM_DIR, trying to update"
    printf "\r=> "
    cd "$NVM_DIR" && (git pull 2> /dev/null || {
      echo >&2 "Failed to update nvm, run 'git pull' in $NVM_DIR yourself.."
    })
  else
    # Cloning to $NVM_DIR
    echo "=> Downloading nvm from git to '$NVM_DIR'"
    printf "\r=> "
    mkdir -p "$NVM_DIR"
    git clone "$NVM_SOURCE" "$NVM_DIR"
  fi
}

install_nvm_as_script() {
  if [ -z "$NVM_SOURCE" ]; then
    NVM_SOURCE="https://raw.githubusercontent.com/creationix/nvm/master/nvm.sh"
  fi

  # Downloading to $NVM_DIR
  mkdir -p "$NVM_DIR"
  if [ -d "$NVM_DIR/nvm.sh" ]; then
    echo "=> nvm is already installed in $NVM_DIR, trying to update"
  else
    echo "=> Downloading nvm as script to '$NVM_DIR'"
  fi
  nvm_download -s "$NVM_SOURCE" -o "$NVM_DIR/nvm.sh" || {
    echo >&2 "Failed to download '$NVM_SOURCE'.."
    return 1
  }
}

if [ -z "$METHOD" ]; then
  # Autodetect install method
  if nvm_has "git"; then
    install_nvm_from_git
  elif nvm_has "nvm_download"; then
    install_nvm_as_script
  else
    echo >&2 "You need git, curl, or wget to install nvm"
    exit 1
  fi
else
  if [ "$METHOD" = "git" ]; then
    if ! nvm_has "git"; then
      echo >&2 "You need git to install nvm"
      exit 1
    fi
    install_nvm_from_git
  fi
  if [ "$METHOD" = "script" ]; then
    if ! nvm_has "nvm_download"; then
      echo >&2 "You need curl or wget to install nvm"
      exit 1
    fi
    install_nvm_as_script
  fi
fi

echo

# Detect profile file if not specified as environment variable (eg: PROFILE=~/.myprofile).
if [ -z "$PROFILE" ]; then
  if [ -f "$HOME/.bash_profile" ]; then
    PROFILE="$HOME/.bash_profile"
  elif [ -f "$HOME/.zshrc" ]; then
    PROFILE="$HOME/.zshrc"
  elif [ -f "$HOME/.profile" ]; then
    PROFILE="$HOME/.profile"
  fi
fi

SOURCE_STR="\nexport NVM_DIR=\"$NVM_DIR\"\n[ -s \"\$NVM_DIR/nvm.sh\" ] && . \"\$NVM_DIR/nvm.sh\"  # This loads nvm"

if [ -z "$PROFILE" ] || [ ! -f "$PROFILE" ] ; then
  if [ -z "$PROFILE" ]; then
    echo "=> Profile not found. Tried ~/.bash_profile, ~/.zshrc, and ~/.profile."
    echo "=> Create one of them and run this script again"
  else
    echo "=> Profile $PROFILE not found"
    echo "=> Create it (touch $PROFILE) and run this script again"
  fi
  echo "   OR"
  echo "=> Append the following lines to the correct file yourself:"
  printf "$SOURCE_STR"
  echo
else
  if ! grep -qc 'nvm.sh' "$PROFILE"; then
    echo "=> Appending source string to $PROFILE"
    echo -e "$SOURCE_STR" >> "$PROFILE"
  else
    echo "=> Source string already in $PROFILE"
  fi
fi

echo "=> Close and reopen your terminal to start using nvm"
