import type { CurveSegment3d, PathGeometry3d, Point3d } from "../model";
import { isCompositeCurve } from "../model";

export interface CurveState3d {
  position: Point3d;
  headingRad: number;
  curvaturePerM: number;
}

export interface Bounds2d {
  minX: number;
  minY: number;
  maxX: number;
  maxY: number;
}

export function normalizeHeading(value: number): number {
  const full = Math.PI * 2;
  return ((value + Math.PI) % full + full) % full - Math.PI;
}

function simpson(fn: (value: number) => number, start: number, end: number): number {
  const middle = (start + end) * 0.5;
  return (end - start) / 6 * (fn(start) + 4 * fn(middle) + fn(end));
}

function adaptiveSimpson(
  fn: (value: number) => number,
  start: number,
  end: number,
  tolerance: number,
  depth = 18,
): number {
  const recurse = (left: number, right: number, expected: number, epsilon: number, remaining: number): number => {
    const middle = (left + right) * 0.5;
    const first = simpson(fn, left, middle);
    const second = simpson(fn, middle, right);
    const delta = first + second - expected;
    if (remaining === 0 || Math.abs(delta) <= 15 * epsilon) return first + second + delta / 15;
    return recurse(left, middle, first, epsilon * 0.5, remaining - 1)
      + recurse(middle, right, second, epsilon * 0.5, remaining - 1);
  };
  return recurse(start, end, simpson(fn, start, end), tolerance, depth);
}

export function evaluateSegment(segment: CurveSegment3d, requestedS: number): CurveState3d {
  const length = Math.max(0, segment.lengthM);
  const s = Math.min(Math.max(requestedS, 0), length);
  const ratio = length > 0 ? s / length : 0;
  const z = segment.start[2] + (segment.endZM - segment.start[2]) * ratio;
  if (segment.type === "line") {
    return {
      position: [segment.start[0] + Math.cos(segment.headingRad) * s, segment.start[1] + Math.sin(segment.headingRad) * s, z],
      headingRad: normalizeHeading(segment.headingRad), curvaturePerM: 0,
    };
  }
  if (segment.type === "circular_arc") {
    const curvature = segment.curvaturePerM;
    if (Math.abs(curvature) <= 1e-12) {
      return {
        position: [segment.start[0] + Math.cos(segment.headingRad) * s, segment.start[1] + Math.sin(segment.headingRad) * s, z],
        headingRad: normalizeHeading(segment.headingRad), curvaturePerM: curvature,
      };
    }
    const heading = segment.headingRad + curvature * s;
    return {
      position: [
        segment.start[0] + (Math.sin(heading) - Math.sin(segment.headingRad)) / curvature,
        segment.start[1] - (Math.cos(heading) - Math.cos(segment.headingRad)) / curvature,
        z,
      ],
      headingRad: normalizeHeading(heading), curvaturePerM: curvature,
    };
  }
  const rate = length > 0 ? (segment.endCurvaturePerM - segment.startCurvaturePerM) / length : 0;
  const headingAt = (distance: number): number => segment.headingRad
    + segment.startCurvaturePerM * distance + 0.5 * rate * distance * distance;
  const tolerance = Math.max(1e-12, Math.abs(s) * 1e-12);
  const x = s === 0 ? 0 : adaptiveSimpson((distance) => Math.cos(headingAt(distance)), 0, s, tolerance);
  const y = s === 0 ? 0 : adaptiveSimpson((distance) => Math.sin(headingAt(distance)), 0, s, tolerance);
  return {
    position: [segment.start[0] + x, segment.start[1] + y, z],
    headingRad: normalizeHeading(headingAt(s)),
    curvaturePerM: segment.startCurvaturePerM + rate * s,
  };
}

export function pathLength(geometry: PathGeometry3d): number {
  if (isCompositeCurve(geometry)) {
    return geometry.segments.reduce((total, segment) => total + Math.max(0, segment.lengthM), 0);
  }
  let total = 0;
  for (let index = 1; index < geometry.length; index += 1) {
    total += Math.hypot(geometry[index][0] - geometry[index - 1][0], geometry[index][1] - geometry[index - 1][1]);
  }
  return total;
}

export function evaluatePath(geometry: PathGeometry3d, requestedS: number): CurveState3d {
  const total = pathLength(geometry);
  const s = Math.min(Math.max(requestedS, 0), total);
  if (isCompositeCurve(geometry)) {
    let accumulated = 0;
    for (let index = 0; index < geometry.segments.length; index += 1) {
      const segment = geometry.segments[index];
      const length = Math.max(0, segment.lengthM);
      if (s <= accumulated + length || index === geometry.segments.length - 1) {
        return evaluateSegment(segment, s - accumulated);
      }
      accumulated += length;
    }
    return { position: [0, 0, 0], headingRad: 0, curvaturePerM: 0 };
  }
  if (geometry.length === 0) return { position: [0, 0, 0], headingRad: 0, curvaturePerM: 0 };
  let accumulated = 0;
  for (let index = 1; index < geometry.length; index += 1) {
    const first = geometry[index - 1];
    const second = geometry[index];
    const length = Math.hypot(second[0] - first[0], second[1] - first[1]);
    if (length <= 1e-12) continue;
    if (s <= accumulated + length || index === geometry.length - 1) {
      const ratio = Math.min(Math.max((s - accumulated) / length, 0), 1);
      return {
        position: [first[0] + (second[0] - first[0]) * ratio, first[1] + (second[1] - first[1]) * ratio, first[2] + (second[2] - first[2]) * ratio],
        headingRad: Math.atan2(second[1] - first[1], second[0] - first[0]), curvaturePerM: 0,
      };
    }
    accumulated += length;
  }
  return { position: geometry[0], headingRad: 0, curvaturePerM: 0 };
}

export function tessellatePath(
  geometry: PathGeometry3d,
  maxChordErrorM = 0.25,
  maxSegmentLengthM = 10,
): Point3d[] {
  if (!isCompositeCurve(geometry)) return geometry.map((point) => [...point] as Point3d);
  const result: Point3d[] = [];
  const append = (segment: CurveSegment3d, startS: number, start: CurveState3d, endS: number, end: CurveState3d, depth: number): void => {
    const middleS = (startS + endS) * 0.5;
    const middle = evaluateSegment(segment, middleS);
    const error = Math.hypot(
      middle.position[0] - (start.position[0] + end.position[0]) * 0.5,
      middle.position[1] - (start.position[1] + end.position[1]) * 0.5,
    );
    if (depth < 20 && (error > Math.max(maxChordErrorM, 1e-9) || endS - startS > Math.max(maxSegmentLengthM, 1e-6))) {
      append(segment, startS, start, middleS, middle, depth + 1);
      append(segment, middleS, middle, endS, end, depth + 1);
      return;
    }
    result.push(end.position);
  };
  for (const segment of geometry.segments) {
    const start = evaluateSegment(segment, 0);
    const previous = result[result.length - 1];
    if (!previous || Math.hypot(previous[0] - start.position[0], previous[1] - start.position[1], previous[2] - start.position[2]) > 1e-6) {
      result.push(start.position);
    }
    if (segment.lengthM > 0) append(segment, 0, start, segment.lengthM, evaluateSegment(segment, segment.lengthM), 0);
  }
  return result;
}

export function pathMidpoint(geometry: PathGeometry3d): Point3d {
  return evaluatePath(geometry, pathLength(geometry) * 0.5).position;
}

export function boundsOfPoints(points: Point3d[]): Bounds2d | undefined {
  if (points.length === 0) return undefined;
  const bounds = { minX: points[0][0], minY: points[0][1], maxX: points[0][0], maxY: points[0][1] };
  for (const point of points) {
    bounds.minX = Math.min(bounds.minX, point[0]); bounds.minY = Math.min(bounds.minY, point[1]);
    bounds.maxX = Math.max(bounds.maxX, point[0]); bounds.maxY = Math.max(bounds.maxY, point[1]);
  }
  return bounds;
}

function squaredDistanceToSegment(point: Point3d, first: Point3d, second: Point3d): number {
  const dx = second[0] - first[0];
  const dy = second[1] - first[1];
  if (dx === 0 && dy === 0) return (point[0] - first[0]) ** 2 + (point[1] - first[1]) ** 2;
  const ratio = Math.min(Math.max(((point[0] - first[0]) * dx + (point[1] - first[1]) * dy) / (dx * dx + dy * dy), 0), 1);
  const x = first[0] + dx * ratio;
  const y = first[1] + dy * ratio;
  return (point[0] - x) ** 2 + (point[1] - y) ** 2;
}

export function simplifyPolyline(points: Point3d[], toleranceM: number): Point3d[] {
  if (points.length <= 2 || toleranceM <= 0) return points.map((point) => [...point] as Point3d);
  const keep = new Uint8Array(points.length);
  keep[0] = 1;
  keep[points.length - 1] = 1;
  const stack: Array<[number, number]> = [[0, points.length - 1]];
  const threshold = toleranceM * toleranceM;
  while (stack.length > 0) {
    const [start, end] = stack.pop()!;
    let farthest = -1;
    let distance = threshold;
    for (let index = start + 1; index < end; index += 1) {
      const candidate = squaredDistanceToSegment(points[index], points[start], points[end]);
      if (candidate > distance) {
        distance = candidate;
        farthest = index;
      }
    }
    if (farthest >= 0) {
      keep[farthest] = 1;
      stack.push([start, farthest], [farthest, end]);
    }
  }
  return points.filter((_, index) => keep[index] === 1).map((point) => [...point] as Point3d);
}
