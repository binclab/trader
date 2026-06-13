all: pull compile run

IP := $(shell ip -4 addr show wlan0 | awk '/inet / {print $$2}' | cut -d/ -f1)

pull:
	git pull

cert:
	openssl req -x509 -new -nodes -keyout rootCA.key -sha256 -days 365 \
	  -out rootCA.crt -subj "/CN=Local Root CA"
	certutil -d sql:$HOME/.pki/nssdb -A -t "CT,C,C" -n "LocalRootCA" -i rootCA.crt
	openssl req -new -nodes -out server.csr -keyout server.key \
	  -subj "/CN=tazi.binclab.com"
	openssl x509 -req -in server.csr -CA rootCA.crt -CAkey rootCA.key \
	  -CAcreateserial -out server.crt -days 365 \
	  -extfile <(printf "subjectAltName=DNS:tazi.binclab.com")


configure:
	meson setup .build

compile:
	meson compile -C .build

run:
	./.build/server

update:
	git add .
	git commit -am "update"
	git push