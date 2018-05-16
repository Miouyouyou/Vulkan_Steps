#ifndef MYY_LOG_HELPERS_H
#define MYY_LOG_HELPERS_H 1

/* I tend to call this function log.
 * But that generate issues when calling math functions... */
#define log_entry(fmt, ...) \
	fprintf(stderr, "[%s:%s():%d]\n" fmt "\n", \
		__FILE__, __func__, __LINE__, ##__VA_ARGS__ )

#endif
