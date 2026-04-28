#!/usr/bin/env bash
set -euo pipefail

CA_NAME="local-dev-ca"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_CA_DIR="${SCRIPT_DIR}/${CA_NAME}"
CA_DIR="${LOCAL_DEV_CA_DIR:-${DEFAULT_CA_DIR}}"
CA_KEY="${CA_DIR}/${CA_NAME}.key"
CA_CRT="${CA_DIR}/${CA_NAME}.crt"

usage() {
  cat <<EOF
Usage:
  LOCAL_DEV_CA_DIR=/tmp/my-ca $0 setup
  $0 issue cert-dir localhost 127.0.0.1 myhost

The CA gets stored next to the script unless you override LOCAL_DEV_CA_DIR.
Use 'issue' to emit server certs with one or more hosts (first host becomes CN).
EOF
}

ensure_ca_dir() {
  if [[ ! -d "${CA_DIR}" ]]; then
    mkdir -p "${CA_DIR}"
  fi
}

generate_ca() {
  ensure_ca_dir
  if [[ -f "${CA_KEY}" && -f "${CA_CRT}" ]]; then
    echo "CA already exists at ${CA_DIR}"
    return
  fi

  openssl genrsa -out "${CA_KEY}" 4096
  openssl req -x509 -new -nodes -key "${CA_KEY}" -sha256 -days 3650 \
    -subj "/C=FR/ST=Some-State/L=SomeCity/O=Local Dev CA/CN=Local Dev CA" \
    -out "${CA_CRT}"
  chmod 600 "${CA_KEY}"
  chmod 644 "${CA_CRT}"
  echo "Local CA generated:"
  echo "  Key: ${CA_KEY}"
  echo "  Cert: ${CA_CRT}"
}

create_san_conf() {
  local tmp="$1"
  shift
  cat <<EOF > "${tmp}"
[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req
prompt = no

[req_distinguished_name]
C = FR
ST = Some-State
L = SomeCity
O = Local Dev Certificate
CN = $1

[v3_req]
subjectAltName = @alt_names

[alt_names]
EOF
  local -i idx=1
  for host in "$@"; do
    if [[ "${host}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
      echo "IP.${idx} = ${host}" >> "${tmp}"
    else
      echo "DNS.${idx} = ${host}" >> "${tmp}"
    fi
    idx=$((idx + 1))
  done
}

issue_cert() {
  local outdir="${1:?}"
  shift
  if [[ $# -lt 1 ]]; then
    echo "issue requires at least one host alias" >&2
    usage
    exit 1
  fi

  generate_ca
  mkdir -p "${outdir}"
  local san_conf
  san_conf="$(mktemp)"
  create_san_conf "${san_conf}" "$@"

  local key="${outdir}/server.key"
  local cert="${outdir}/server.crt"
  local csr="${outdir}/server.csr"

  openssl genrsa -out "${key}" 2048
  openssl req -new -key "${key}" -out "${csr}" -config "${san_conf}"
  openssl x509 -req -in "${csr}" -CA "${CA_CRT}" -CAkey "${CA_KEY}" \
    -CAcreateserial -out "${cert}" -days 365 -sha256 -extensions v3_req \
    -extfile "${san_conf}"

  rm -f "${csr}" "${san_conf}"

  echo "Issued certificate signed by local CA:"
  echo "  Key : ${key}"
  echo "  Cert: ${cert}"
}

main() {
  if [[ $# -lt 1 ]]; then
    usage
    exit 1
  fi

  case "$1" in
    setup)
      generate_ca
      ;;
    issue)
      shift
      issue_cert "$@"
      ;;
    *)
      usage
      exit 1
      ;;
  esac
}

main "$@"
