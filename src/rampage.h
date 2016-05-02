struct Rmpg *init_rampage(void);
struct Rmpg {
	/* Contains all of the server options. These can be called by the user by
	 * calling rampage_setopt after initialization. */
	struct Options *options;
	struct lws_context_creation_info *info;
};
