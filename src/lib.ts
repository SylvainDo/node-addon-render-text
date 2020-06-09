export type rgb = { r: number, g: number, b: number };
export type rgba32v = Uint8Array;
export type Surface = { w: number, h: number, data: rgba32v };

let mod = require('@render-text-module');

export var renderText = mod.renderText as (fontPath: string, fontSize: number, text: string,
    fillColor: rgb, outlineColor: rgb, outlineThickness: number) => Surface;

export var renderTexts = mod.renderTexts as (fontPath: string, fontSize: number, texts: string[],
    fillColor: rgb, outlineColor: rgb, outlineThickness: number) => Surface[];

export var savePng = mod.savePng as (path: string, surface: Surface) => void;
