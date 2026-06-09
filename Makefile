all: cert compile

IP := $(shell ip -4 addr show wlan0 | awk '/inet / {print $$2}' | cut -d/ -f1)

cert:
	echo "subjectAltName=IP:$(IP)" > /var/www/ext.cnf
	openssl req -new -key /var/www/server.key -out /var/www/cert.csr -subj "/CN=$(IP)"
	openssl x509 -req -days 365 -in /var/www/cert.csr -signkey /var/www/server.key \
	   -out /var/www/cert.pem -extfile /var/www/ext.cnf

configure:
	meson setup .build

compile:
	meson compile -C .build
