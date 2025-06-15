# SubvisionCV

Bibliothèque de détection d'impacts sur cible de tir sportif utilisant OpenCV.

## Compilation native

```bash
mkdir build && cd build
cmake ..
make
```

## Compilation avec Emscripten

### Prérequis

1. Installez Emscripten SDK : [Instructions d'installation](https://emscripten.org/docs/getting_started/downloads.html)

2. Compilez OpenCV pour Emscripten :

```bash
# Cloner OpenCV
git clone https://github.com/opencv/opencv.git
cd opencv
mkdir build_wasm
cd build_wasm

# Configurer et compiler avec Emscripten
emcmake cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/chemin/vers/votre/projet/opencv_emscripten_build \
    -DBUILD_opencv_dnn=OFF \
    -DBUILD_opencv_ml=OFF \
    -DBUILD_opencv_video=OFF \
    -DBUILD_opencv_stitching=OFF \
    -DBUILD_opencv_objdetect=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    ..

emmake -j8
emmake install
```

### Compilation du projet

```bash
mkdir build_wasm && cd build_wasm
emcmake cmake ..
emmake
```

### Exécution du serveur de test

```bash
python3 -m http.server
```

Puis ouvrez votre navigateur à l'adresse http://localhost:8000 pour tester l'application.

## Utilisation dans un projet JavaScript

```javascript
// Importation du module WebAssembly
import SubvisionCVModule from './subvision_core.js';

// Initialisation du module
SubvisionCVModule().then(subvisionCV => {
    // Analyse d'une image en base64
    const imageBase64 = 'data:image/jpeg;base64,...'; // Remplacer par votre image en base64

    // Appel de la fonction de détection d'impacts
    const results = subvisionCV.processTargetImage(imageBase64);

    // Utilisation des résultats
    console.log(`Nombre d'impacts détectés: ${results.impacts.length}`);

    // Image annotée (base64)
    const annotatedImage = results.annotatedImage;

    // Liste des impacts
    results.impacts.forEach(impact => {
        console.log(`Zone: ${impact.zone}, Score: ${impact.score}`);
    });
});
```
