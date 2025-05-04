import { readFileSync, writeFileSync } from 'fs';
import { resolve } from 'path';

const distPath = resolve('./dist', 'index.html');
let html = readFileSync(distPath, 'utf8');

// Inject a script before closing body
html = html.replace('<script type="module" crossorigin src="/assets', '<script type="module" crossorigin src="./assets');

writeFileSync(distPath, html, 'utf8');
console.log('Modified index.html after build');
