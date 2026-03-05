# JavaScript / WebAssembly Usage Guide

This guide covers how to use Subvision CV in web applications via the Emscripten WebAssembly module.

## Overview

Subvision CV is compiled to WebAssembly using Emscripten and exposed to JavaScript through `embind`. The module provides two main functions:

| Function | Purpose |
|----------|---------|
| `processTargetImage(width, height, data)` | Detect and score all impacts on a shooting sheet |
| `getSheetCoordinates(width, height, data)` | Detect the sheet corner coordinates |
| `setLoggingEnabled(bool)` | Enable/disable console logging |

## Loading the Module

### ES6 Module (Recommended)

```javascript
import SubvisionCV from './subvision_core_es6.js';

// Initialize the module (loads the WASM binary)
const module = await SubvisionCV();
console.log('Subvision CV loaded successfully');
```

### Standard Script

```html
<script src="subvision_core.js"></script>
<script>
    Subvision().then(module => {
        console.log('Subvision CV loaded successfully');
    });
</script>
```

## Core API

### Processing a Target Image

The `processTargetImage` function takes raw pixel data from a canvas and returns impact detection results.

```javascript
async function processImage(imageElement) {
    const module = await SubvisionCV();

    // Draw image to a canvas to get pixel data
    const canvas = document.createElement('canvas');
    canvas.width = imageElement.naturalWidth;
    canvas.height = imageElement.naturalHeight;
    const ctx = canvas.getContext('2d');
    ctx.drawImage(imageElement, 0, 0);

    // Get RGBA pixel data
    const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

    // Process the image
    const results = module.processTargetImage(
        canvas.width, canvas.height, imageData.data
    );

    // Read results
    console.log('Annotated image dimensions:',
        results.annotatedImage.rows, 'x', results.annotatedImage.columns);

    const impactCount = results.impacts.size();
    console.log(`Detected ${impactCount} impacts:`);

    for (let i = 0; i < impactCount; i++) {
        const impact = results.impacts.get(i);
        console.log(`  Impact ${i + 1}:`,
            `score=${impact.score}`,
            `distance=${impact.distance}mm`,
            `zone=${impact.zone}`,
            `angle=${impact.angle}°`
        );
    }

    return results;
}
```

### Displaying the Annotated Image

```javascript
function displayAnnotatedImage(results, targetCanvas) {
    const mat = results.annotatedImage;
    const width = mat.columns;
    const height = mat.rows;

    targetCanvas.width = width;
    targetCanvas.height = height;
    const ctx = targetCanvas.getContext('2d');

    // Get pixel data from the Mat (RGBA format)
    const data = new Uint8ClampedArray(mat.data);
    const imageData = new ImageData(data, width, height);

    ctx.putImageData(imageData, 0, 0);
}
```

### Detecting Sheet Coordinates

Use `getSheetCoordinates` to find the sheet corners. These can be cached and reused.

```javascript
async function detectSheet(imageElement) {
    const module = await SubvisionCV();

    const canvas = document.createElement('canvas');
    canvas.width = imageElement.naturalWidth;
    canvas.height = imageElement.naturalHeight;
    const ctx = canvas.getContext('2d');
    ctx.drawImage(imageElement, 0, 0);

    const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

    // Returns an array of {x, y} points (normalised 0-1 coordinates)
    const coords = module.getSheetCoordinates(
        canvas.width, canvas.height, imageData.data
    );

    console.log('Sheet corners:');
    for (let i = 0; i < coords.length; i++) {
        console.log(`  Corner ${i}: (${coords[i].x}, ${coords[i].y})`);
    }

    return coords;
}
```

## Data Types

### Impact Object

| Property | Type | Description |
|----------|------|-------------|
| `distance` | `number` | Distance from target center (mm) |
| `score` | `number` | Score on 0–570 FFESSM scale |
| `zone` | `number` | Target zone ID (0–4, or -1) |
| `angle` | `number` | Angular position in degrees |
| `count` | `number` | Impact count (usually 1) |

### ImpactResults Object

| Property | Type | Description |
|----------|------|-------------|
| `annotatedImage` | `Mat` | Annotated image with drawn targets and scores |
| `impacts` | `ImpactVector` | Vector of Impact objects |

### Mat Object

| Property | Type | Description |
|----------|------|-------------|
| `rows` | `number` | Image height in pixels |
| `columns` | `number` | Image width in pixels |
| `data` | `Uint8Array` | Raw RGBA pixel data |

### ImpactVector Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `.size()` | `number` | Number of impacts |
| `.get(index)` | `Impact` | Impact at the given index |

## Performance Considerations

- **Image size**: Larger images take longer. The library internally resizes to 2000×2000 pixels, so there is no benefit to providing images larger than this.
- **First call**: The first call may be slower due to WASM compilation. Subsequent calls are faster.
- **Memory**: Use `Module._malloc` and `Module._free` carefully if working with raw memory. The embind API handles memory automatically for the exported functions.
- **Web Workers**: For non-blocking UI, run the processing in a Web Worker.

## Error Handling

```javascript
try {
    const results = module.processTargetImage(width, height, data);
    if (results.impacts.size() === 0) {
        console.warn('No impacts detected — the sheet may not be visible');
    }
} catch (error) {
    console.error('Processing failed:', error.message);
    // Common causes:
    // - Image does not contain a visible shooting sheet
    // - Image is too dark or overexposed
    // - Sheet is partially occluded
}
```

## Debugging

Enable logging to see the internal processing steps in the browser console:

```javascript
module.setLoggingEnabled(true);

// Process image — detailed logs will appear in the console
const results = module.processTargetImage(width, height, data);

// Disable logging when done
module.setLoggingEnabled(false);
```

## Complete Working Example

```html
<!DOCTYPE html>
<html>
<head>
    <title>Subvision CV Demo</title>
</head>
<body>
    <input type="file" id="imageInput" accept="image/*" />
    <canvas id="resultCanvas"></canvas>
    <div id="scores"></div>

    <script type="module">
        import SubvisionCV from './subvision_core_es6.js';

        const module = await SubvisionCV();

        document.getElementById('imageInput').addEventListener('change', async (e) => {
            const file = e.target.files[0];
            if (!file) return;

            const img = new Image();
            img.onload = () => {
                // Get pixel data
                const canvas = document.createElement('canvas');
                canvas.width = img.width;
                canvas.height = img.height;
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0);
                const imageData = ctx.getImageData(0, 0, img.width, img.height);

                // Process
                const results = module.processTargetImage(
                    img.width, img.height, imageData.data
                );

                // Display annotated image
                const resultCanvas = document.getElementById('resultCanvas');
                const mat = results.annotatedImage;
                resultCanvas.width = mat.columns;
                resultCanvas.height = mat.rows;
                const rCtx = resultCanvas.getContext('2d');
                const outData = new ImageData(
                    new Uint8ClampedArray(mat.data),
                    mat.columns, mat.rows
                );
                rCtx.putImageData(outData, 0, 0);

                // Display scores
                const scoresDiv = document.getElementById('scores');
                let html = '<h3>Results</h3><ul>';
                for (let i = 0; i < results.impacts.size(); i++) {
                    const imp = results.impacts.get(i);
                    html += `<li>Impact ${i+1}: Score=${imp.score}, ` +
                            `Distance=${imp.distance}mm, Zone=${imp.zone}</li>`;
                }
                html += '</ul>';
                scoresDiv.innerHTML = html;
            };
            img.src = URL.createObjectURL(file);
        });
    </script>
</body>
</html>
```
