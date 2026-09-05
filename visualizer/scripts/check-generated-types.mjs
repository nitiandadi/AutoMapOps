import { compileFromFile } from "json-schema-to-typescript";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const schemaPath = resolve(projectRoot, "..", "schemas", "canonical-map-1.1.schema.json");
const outputPath = resolve(projectRoot, "src", "generated", "canonical-map.generated.ts");
const expected = await compileFromFile(schemaPath, {
  bannerComment: "/* 此文件由 npm run generate:types 生成，请勿手工修改。 */",
  style: { singleQuote: false, semi: true },
});
const actual = await readFile(outputPath, "utf8");
if (actual !== expected) {
  throw new Error("Canonical JSON Schema 与生成的 TypeScript 类型不一致，请运行 npm run generate:types。");
}
console.log("Canonical TypeScript 类型与 JSON Schema 一致。");
