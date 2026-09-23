plugins {
    id("com.android.application")
}

android {
    namespace = "com.example.modernimgui"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.example.modernimgui"
        minSdk = 21
        targetSdk = 29
        versionCode = 1
        versionName = "3.2"

        ndk {
            abiFilters += listOf("armeabi-v7a", "arm64-v8a")
        }
    }

    ndkVersion = "27.0.12077973"

    externalNativeBuild {
        ndkBuild {
            path = file("src/main/cpp/Android.mk")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}