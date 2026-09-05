import logisticsPark from "../../maps/drafts/logistics_park_from_opendrive_v1_1.json";
import type { MapData } from "./model";

// 内置入口直接使用由 OpenDRIVE 1.8 翻译得到的正式 Canonical 1.1 示例。
export const sampleMap = logisticsPark as unknown as MapData;
