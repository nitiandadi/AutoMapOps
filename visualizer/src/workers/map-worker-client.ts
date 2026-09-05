import type { MapData } from "../model";
import type { PreparedMap, WorkerRequest, WorkerResponse } from "../render/types";

export class MapWorkerClient {
  private worker?: Worker;
  private requestId = 0;

  loadText(text: string, source: string): Promise<PreparedMap> {
    return this.request({ requestId: ++this.requestId, source, text });
  }

  loadMap(map: MapData, source: string): Promise<PreparedMap> {
    return this.request({ requestId: ++this.requestId, source, map });
  }

  dispose(): void {
    this.worker?.terminate();
    this.worker = undefined;
  }

  private request(message: WorkerRequest): Promise<PreparedMap> {
    this.dispose();
    const worker = new Worker(new URL("./map-worker.ts", import.meta.url), { type: "module" });
    this.worker = worker;
    return new Promise((resolve, reject) => {
      worker.onmessage = (event: MessageEvent<WorkerResponse>) => {
        if (event.data.requestId !== message.requestId) return;
        this.dispose();
        if (event.data.ok) resolve(event.data.result);
        else reject(new Error(event.data.error));
      };
      worker.onerror = (event) => {
        this.dispose();
        reject(new Error(event.message || "地图 Worker 执行失败"));
      };
      worker.postMessage(message);
    });
  }
}
