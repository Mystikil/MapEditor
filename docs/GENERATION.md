# Procedural map generation

The **Tools → Generate Map…** command creates a brand-new map using the deterministic generator that lives under `source/generator/`.  Maps are produced chunk-by-chunk so large worlds stay responsive and a cancellable progress bar is shown while work is in flight.

## Generator options

The dialog collects the same values that `GenOptions` exposes in `generator/MapGenerator.h`:

| Option | Description |
| --- | --- |
| **Seed** | 64-bit seed forwarded to the noise, RNG, and every feature pass so worlds are reproducible. |
| **Width / Height** | Final map dimensions in tiles. The generator works in `chunkSize` slices (default 256) to control memory usage. |
| **Overworld Z** | Floor that receives the overworld terrain. |
| **Cave Z** | Floor carved by the cave cellular-automata pass. Dungeons are placed on the floor just below (`caveZ + 1`). |
| **Biome Preset** | JSON preset loaded from `data/generator/biomes/`. See below for the file format. |
| **Generate Rivers** | Enables the downhill river tracer. |
| **Generate Caves** | Runs the 2D cellular automata to carve caves. |
| **Generate Dungeons** | Adds a BSP-style dungeon layout on the floor below the caves. |
| **Place Starter Town** | Clears a plaza around map centre, drops the preset’s `temple_tile`, and seeds a starter town entry. |

The command always writes the result into a brand-new editor tab and marks the map as modified so it can be saved through the existing **File → Save** flow.

## Biome presets

Presets are regular JSON files.  The generator looks in `data/generator/biomes/` and lists every `*.json` it finds.  A preset provides:

```json
{
  "sea_level": 0.45,
  "mountain_level": 0.8,
  "tiles": {
    "deep_water": 4608,
    "shallow_water": 4820,
    "sand": 231,
    "grass": 4526,
    "rock": 918,
    "snow": 6594,
    "dirt": 103
  },
  "decor": [
    { "name": "tree", "tile": 2701, "density": 0.0075 }
  ],
  "spawn": { "temple_tile": 459, "radius": 6 }
}
```

* `sea_level` and `mountain_level` drive terrain thresholds.
* `tiles` supplies the ground item IDs for each major biome slot.  Never hardcode IDs in code—new tiles only require updating the JSON.
* `decor` is a list of optional decorations. `density` is the probability threshold checked against a deterministic RNG.  Name strings are advisory and let the code bias densities (for example wetter ground favours “tree” entries).
* `spawn` is optional—if present the generator clears an area and places the `temple_tile` with the given radius.

Create new biomes by duplicating an existing preset, adjusting the tile IDs, and tweaking the thresholds/densities to match the desired climate.

## Headless usage

`MapGenerator::Generate` accepts an optional progress callback.  It can be reused from CLI code (see `GenOptions`) to implement headless generation pipelines.  The callback is invoked for every chunk and feature stage and should return `false` to abort generation.

