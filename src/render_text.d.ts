export type rgb = { r: number, g: number, b: number };
export type rgba32v = Uint8Array;
export type Surface = { w: number, h: number, data: rgba32v };

export function renderText(fontPath: string, fontSize: number, text: string,
    fillColor: rgb, outlineColor: rgb, outlineThickness: number): Surface;

export function renderTexts(fontPath: string, fontSize: number, texts: string[],
    fillColor: rgb, outlineColor: rgb, outlineThickness: number): Surface[];

export function savePng(path: string, surface: Surface): void;
