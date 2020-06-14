const fs = require('fs');
const path = require('path');

fs.copyFileSync(
    path.join(__dirname, `../build/Release/render_text.node`),
    path.join(__dirname, `../js/render_text.node`)
    );

fs.copyFileSync(
    path.join(__dirname, `../src/render_text.d.ts`),
    path.join(__dirname, `../js/render_text.d.ts`)
    );
