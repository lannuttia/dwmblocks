#!/bin/sh

set -e

setup_color() {
	# Only use colors if connected to a terminal
    if [ -t 1 ]; then
		RED=$(printf '\033[31m')
		GREEN=$(printf '\033[32m')
		YELLOW=$(printf '\033[33m')
		BLUE=$(printf '\033[34m')
		BOLD=$(printf '\033[1m')
		RESET=$(printf '\033[m')
	else
		RED=""
		GREEN=""
		YELLOW=""
		BLUE=""
		BOLD=""
		RESET=""
	fi
}

error() {
	echo ${RED}"Error: $@"${RESET} >&2
}

if [ -f /etc/os-release ] || [ -f /usr/lib/os-release ] || [ -f /etc/openwrt_release ] || [ -f /etc/lsb_release ]; then
   for file in /etc/os-release /usr/lib/os-release /etc/openwrt_release /etc/lsb_release; do
     [ -f "$file" ] && . "$file" && break
   done
else
  error 'Failed to sniff environment'
  exit 1
fi

if [ $ID_LIKE ]; then
  os=$ID_LIKE
else
  os=$ID
fi

command_exists() {
	command -v "$@" >/dev/null 2>&1
}

run_as_root() {
  if [ "$EUID" = 0 ]; then
    eval "$*"
  elif command_exists sudo; then
    sudo -v
    if [ $? -eq 0 ]; then
      eval "sudo sh -c '$*'"
    else
      su -c "$*"
    fi
  else
    su -c "$*"
  fi
}

usage() {
  echo "Usage: $0 [OPTIONS]"
  echo
  echo "OPTIONS:"
  echo -e "\t--help\t\t\tDisplay this help menu"
}

update() {
  case $os in
    debian|ubuntu)
      run_as_root apt update
    ;;
    alpine)
      run_as_root apk update
    ;;
    arch|artix)
      run_as_root pacman -Sy
    ;;
    *)
      error "Unsupported Distribution: $os"
      exit 1
    ;;
  esac
}

packages() {
  case $ID in
    kali)
      case $VERSION_ID in
        *)
          echo -n ' make gcc libx11-dev pkgconf libxft-dev'
        ;;
      esac
    ;;
    ubuntu|elementary)
      case $VERSION_ID in
        18.04|5.*)
          echo -n ' make gcc libx11-dev pkgconf libxft-dev'
        ;;
        20.04)
          echo -n ' make gcc libx11-dev pkgconf libxft-dev'
        ;;
        *)
          error "Unsupported version of $NAME: $VERSION_ID"
          exit 1;
        ;;
      esac
    ;;
    debian)
      case $VERSION_ID in
        10)
          echo -n ' make gcc libx11-dev pkgconf libxft-dev'
        ;;
        9)
          echo -n ' make gcc libx11-dev pkgconf libxft-dev'
        ;;
        *)
          error "Unsupported version of $NAME: $VERSION_ID"
        ;;
      esac
    ;;
    arch|artix)
      echo -n ' make gcc pkgconf libxft'
    ;;
    *)
      error "Unsupported OS: $NAME"
      exit 1
    ;;
  esac
}

install() {
  case $os in
    debian|ubuntu)
      run_as_root apt install -y $(packages)
    ;;
    arch|artix)
      run_as_root pacman -S --noconfirm $(packages)
    ;;
    alpine)
      run_as_root apk add $(packages)
    ;;
    *)
      error "Unsupported OS: $NAME"
      exit 1
    ;;
  esac
}

main() {

  # Transform long options to short options
  while [ $# -gt 0 ]; do
    case $1 in
      --help) usage; exit 0 ;;
      *) usage >&2; exit 1 ;;
    esac
    shift
  done

  setup_color
  update
  install
}

main "$@"
