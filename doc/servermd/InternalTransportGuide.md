# Open WebRTC Toolkit Internal Transport

## How to enable TLS

Place `server.crt`, `server.key`, `dh2048.pem` under `cert` directory.

## OpenSSL example for certificate files

// Generate a private key
openssl genrsa -des3 -out server.key 1024

// Generate Certificate signing request
openssl req -new -key server.key -out server.csr

// Sign certificate with private key
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

// Generate dhparam file
openssl dhparam -out dh2048.pem 2048
