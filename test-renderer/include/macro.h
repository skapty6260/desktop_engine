#pragma once

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

uint32_t clamp_uint32_t(uint32_t n, uint32_t min, uint32_t max) {
  if (n < min) return min;
  if (n > max) return max;
  return n;
}