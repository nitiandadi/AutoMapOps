import { compileFromFile } from "json-schema-to-typescript";
import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const schemaPath = resolve(projectRoot, "..", "schemas", "canonical-map-1.1.schema.json");
const outputPath = resolve(projectRoot, "src", "generated", "canonical-map.generated.ts");
const source = await compileFromFile(schemaPath, {
  bannerComment: "/* 此文件由 npm run generate:types 生成，请勿手工修改。 */",
  style: { singleQuote: false, semi: true },
});

await mkdir(dirname(outputPath), { recursive: true });
await writeFile(outputPath, source, "utf8");
console.log(`已生成 ${outputPath}`);
