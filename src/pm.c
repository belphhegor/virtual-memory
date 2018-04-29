#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init(FILE * backing_store, FILE * log)
{
	pm_backing_store = backing_store;
	pm_log = log;
	memset(pm_memory, '\0', sizeof(pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page(unsigned int page_number, unsigned int frame_number)
{
	if (page_number >= NUM_PAGES || frame_number >= NUM_FRAMES) {
		fprintf(stderr, "PM: invalid page or frame number\n");
		return;
	}

	if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET) <
	    0) {
		perror("PM");
		return;
	}

	/* XXX Copy to buffer first? */
	fread(pm_memory + frame_number * PAGE_FRAME_SIZE,
	      PAGE_FRAME_SIZE, 1, pm_backing_store);

	if (ferror(pm_backing_store)) {
		fprintf(stderr, "PM: error in backing store\n");
		return;
	}

	download_count++;
}

// Sauvegarde le frame spécifiée dans la page du backing store
void pm_backup_page(unsigned int frame_number, unsigned int page_number)
{
	if (page_number >= NUM_PAGES || frame_number >= NUM_FRAMES) {
		fprintf(stderr, "PM: invalid page or frame number\n");
		return;
	}

	if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET) <
	    0) {
		perror("PM");
		return;
	}

	/* XXX Copy to buffer first? */
	fwrite(pm_memory + frame_number * PAGE_FRAME_SIZE,
	       PAGE_FRAME_SIZE, 1, pm_backing_store);

	if (ferror(pm_backing_store)) {
		fprintf(stderr, "PM: error in backing store\n");
		return;
	}

	backup_count++;
}

char pm_read(unsigned int physical_address)
{
	if (physical_address >= PHYSICAL_MEMORY_SIZE) {
		fprintf(stderr, "PM: invalid physical address\n");
		return '!';
	}

	read_count++;
	return pm_memory[physical_address];
}

void pm_write(unsigned int physical_address, char c)
{
	if (physical_address >= PHYSICAL_MEMORY_SIZE) {
		fprintf(stderr, "PM: invalid physical address\n");
		return;
	}

	write_count++;
	pm_memory[physical_address] = c;
}

void pm_clean(void)
{
	// XXX Final backup and flush
	// https://studium.umontreal.ca/mod/forum/discuss.php?d=548447

	// Enregistre l'état de la mémoire physique.
	if (pm_log) {
		for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
			if (i % 80 == 0)
				fprintf(pm_log, "%c\n", pm_memory[i]);
			else
				fprintf(pm_log, "%c", pm_memory[i]);
		}
	}
	fprintf(stdout, "Page downloads: %2u\n", download_count);
	fprintf(stdout, "Page backups  : %2u\n", backup_count);
	fprintf(stdout, "PM reads : %4u\n", read_count);
	fprintf(stdout, "PM writes: %4u\n", write_count);
}
