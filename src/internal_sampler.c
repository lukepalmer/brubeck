#include "brubeck.h"

#define INTERNAL_LONGEST_KEY ".unique_keys"

void brubeck_internal__sample(struct brubeck_metric *metric,
                              brubeck_sample_cb sample, void *opaque) {
  struct brubeck_internal_stats *stats = metric->as.other;
  uint32_t value;

  char *key = alloca(metric->key_len + strlen(INTERNAL_LONGEST_KEY) + 1);
  memcpy(key, metric->key, metric->key_len);

  WITH_SUFFIX(".metrics") {
    value = brubeck_atomic_swap(&stats->live.metrics, 0);
    stats->sample.metrics = value;
    sample(metric, key, (value_t)value, opaque);
  }

  WITH_SUFFIX(".errors") {
    value = brubeck_atomic_swap(&stats->live.errors, 0);
    stats->sample.errors = value;
    sample(metric, key, (value_t)value, opaque);
  }

  WITH_SUFFIX(".unique_keys") {
    value = brubeck_atomic_fetch(&stats->live.unique_keys);
    stats->sample.unique_keys = value;
    sample(metric, key, (value_t)value, opaque);
  }

  /*
   * Mark the metric as active so it doesn't get disabled
   * by the inactive metrics pruner
   */
  brubeck_metric_set_state(metric, BRUBECK_STATE_ACTIVE);
}

void brubeck_internal__init(struct brubeck_server *server) {
  struct brubeck_metric *internal;
  struct brubeck_backend *backend;

  internal = brubeck_metric_new(server, server->name, strlen(server->name),
                                BRUBECK_MT_INTERNAL_STATS);

  if (internal == NULL)
    die("Failed to initialize internal stats sampler");

  internal->as.other = &server->internal_stats;

  backend = brubeck_metric_shard(server, internal);
  server->internal_stats.sample_freq = backend->sample_freq;
}
