import { expect, test } from "@playwright/test";
import path from "node:path";

test("加载 OpenDRIVE 物流园转换示例并同步对象检查器", async ({ page }, testInfo) => {
  await page.goto("/");
  await expect(page.getByText("物流园 OpenDRIVE 1.8 转换示例")).toBeVisible();
  await page.getByRole("button", { name: /园区入口路/ }).click();
  await expect(page.getByRole("dialog", { name: "对象属性" })).toBeVisible();
  await expect(page.locator("#object-json")).toHaveCount(0);
  const popupBounds = await page.getByRole("dialog", { name: "对象属性" }).boundingBox();
  const mapBounds = await page.locator(".map-shell").boundingBox();
  expect(popupBounds!.x).toBeGreaterThanOrEqual(mapBounds!.x);
  expect(popupBounds!.x + popupBounds!.width).toBeLessThanOrEqual(mapBounds!.x + mapBounds!.width);
  expect(popupBounds!.y + popupBounds!.height).toBeLessThanOrEqual(mapBounds!.y + mapBounds!.height);
  await page.screenshot({ path: testInfo.outputPath("property-popup.png") });
  await page.getByRole("button", { name: "原始 JSON", exact: true }).click();
  await expect(page.getByRole("heading", { name: "Road · road_entry" })).toBeVisible();
  await expect(page.locator("#object-json")).toContainText("composite_curve");
  await page.getByRole("button", { name: "属性表", exact: true }).click();
  await page.getByRole("dialog", { name: "对象属性" }).getByRole("button", { name: "lane_entry_inner", exact: true }).click();
  await expect(page.getByRole("dialog", { name: "对象属性" }).getByRole("heading", { name: "lane_entry_inner", exact: true })).toBeVisible();
  await page.getByRole("button", { name: "关闭属性弹窗" }).click();
  await expect(page.getByRole("dialog", { name: "对象属性" })).toHaveCount(0);
});

test("无几何对象使用固定属性卡片并记住查询模式", async ({ page }) => {
  await page.goto("/");
  await page.getByRole("searchbox").fill("vehicle_truck_12m");
  await page.getByRole("button", { name: /VehicleProfile · vehicle_truck_12m/ }).click();
  await expect(page.getByRole("dialog", { name: "对象属性" })).toBeVisible();
  await page.getByRole("button", { name: "原始 JSON", exact: true }).click();
  await page.reload();
  await expect(page.getByRole("button", { name: "原始 JSON", exact: true })).toHaveAttribute("aria-pressed", "true");
});

test("从磁盘打开 OpenDRIVE 转换结果并绘制完整园区", async ({ page }) => {
  await page.goto("/");
  const jsonPath = path.resolve("../maps/drafts/logistics_park_from_opendrive_v1_1.json");
  await page.locator("#map-file").setInputFiles(jsonPath);

  await expect(page.getByText("物流园 OpenDRIVE 1.8 转换示例")).toBeVisible();
  await expect(page.getByText(/logistics_park_from_opendrive_v1_1\.json · 83 个对象 · 64 条路径/)).toBeVisible();
  await expect(page.getByRole("button", { name: /园区入口路/ })).toBeVisible();
  await expect(page.locator("canvas")).toBeVisible();
  const zoom = Number(await page.getByLabel("Canonical 地图 WebGL 画布").getAttribute("data-view-zoom"));
  const viewportZoom = Number(await page.getByLabel("Canonical 地图 WebGL 画布").getAttribute("data-viewport-zoom"));
  expect(Number.isFinite(zoom)).toBe(true);
  expect(Number.isFinite(viewportZoom)).toBe(true);
  expect(zoom).toBeGreaterThan(0);
  expect(viewportZoom).toBeGreaterThan(0);
});
