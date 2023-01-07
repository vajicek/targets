APK_FILE=targets_app/app/build/outputs/apk/release/app-release-unsigned.apk
STORE_PASS=abcdef12
KEY_PASS=abcdef12
KEYSTORE_FILE=targets-upload.keystore
KEYSTORE_ALIAS=targets-upload
BUILD_TOOLS_VERSION=30.0.2
BUILD_TOOLS_PATH=${HOME}/Android/Sdk/build-tools/${BUILD_TOOLS_VERSION}

clean_app:
	rm $(APK_FILE)

build_app: $(APK_FILE)

$(APK_FILE):
	cd targets_app && ./gradlew clean build

train_model:
	echo "TODO"

deploy_app:
	echo "TODO"

generate_upload_key_keystore:
	sudo keytool \
		-genkey \
		-v \
		-keystore $(KEYSTORE_FILE) \
		-alias $(KEYSTORE_ALIAS) \
		-keyalg RSA \
		-keysize 2048 \
		-dname "CN=Vaclav Krajicek, OU=Development, O=VajSoft, L=Prague, ST=Prague, C=CZ" \
		-validity 10000 \
		-storepass $(STORE_PASS) \
		-keypass $(KEY_PASS)

sign_app: $(APK_FILE)
	$(BUILD_TOOLS_PATH)/apksigner sign \
		--ks-pass pass:$(STORE_PASS) \
		--key-pass pass:$(KEY_PASS) \
		--ks targets-upload.keystore $(APK_FILE)

install_app_on_device: sign_app
	$(eval DEVICE := $(shell adb devices| sed -n '2p'|cut -f1))
	adb -s "$(DEVICE)" install "$(APK_FILE)"