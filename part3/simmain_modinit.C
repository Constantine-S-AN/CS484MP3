extern void _registerCommonLBs(void);

void _registerExternalModules(char **argv) {
	(void)argv;
	_registerCommonLBs();
}

void _createTraces(char **argv) {
	(void)argv;
}
