/**
 * Utility functions for SVG zoom tests using testharness.js
 */

/**
 * Wait for object elements to load and have dimensions
 * @param {string[]} objectIds - Array of element IDs to wait for
 * @param {number} timeout - Timeout in milliseconds (default 5000)
 * @returns {Promise<void>}
 */
async function waitForObjectsToLoad(objectIds, timeout = 5000) {
    const startTime = Date.now();

    while (Date.now() - startTime < timeout) {
        let allLoaded = true;

        for (const id of objectIds) {
            const element = document.getElementById(id);
            if (!element) {
                allLoaded = false;
                break;
            }

            const rect = element.getBoundingClientRect();
            if (rect.width === 0 || rect.height === 0) {
                allLoaded = false;
                break;
            }
        }

        if (allLoaded) {
            // Give one more frame for rendering to complete
            await new Promise(resolve => requestAnimationFrame(resolve));
            return;
        }

        await new Promise(resolve => requestAnimationFrame(resolve));
    }

    // Timeout reached, continue anyway
}

/**
 * Get dimensions of an element
 * @param {string} elementId - Element ID
 * @returns {object} Object with width and height properties
 */
function getElementDimensions(elementId) {
    const element = document.getElementById(elementId);
    const rect = element.getBoundingClientRect();
    return {
        width: rect.width,
        height: rect.height
    };
}

/**
 * Zoom the page out
 * @param {number} steps - Number of zoom-out steps
 * @returns {Promise<void>}
 */
async function zoomPageOut(steps = 1) {
    if (!window.eventSender) {
        return;
    }

    for (let i = 0; i < steps; i++) {
        await window.eventSender.zoomPageOut();
    }

    // Wait for zoom to take effect
    await new Promise(resolve => {
        const checkZoom = () => {
            // Just wait one frame and assume zoom took effect
            resolve();
        };
        requestAnimationFrame(checkZoom);
    });
}

/**
 * Wait for zoom to affect an element's dimensions
 * @param {string} elementId - Element ID to monitor
 * @param {number} initialWidth - Initial width before zoom
 * @returns {Promise<void>}
 */
async function waitForZoomEffect(elementId, initialWidth) {
    return new Promise(resolve => {
        const checkZoom = () => {
            const element = document.getElementById(elementId);
            const rect = element.getBoundingClientRect();
            // Check if zoom has taken effect (width should have changed)
            if (rect.width < initialWidth) {
                resolve();
            } else {
                requestAnimationFrame(checkZoom);
            }
        };
        checkZoom();
    });
}
