struct Rmpg *init_rampage(void);
struct Rmpg {
	/* Contains all of the server options. These can be called by the user by
	 * calling rampage_setopt after initialization. */
	struct Options *options;
	struct lws_context_creation_info *info;
};
//
// struct Options {
// 	const char *db_path;
// 	const char *resource_path;
// 	const char *cert_path;
// 	const char *key_path;
// 	const char *interface_name;
// 	const char *iface;
// 	short int use_ssl;
// 	int syslog_options;
// 	int debug_level;
// 	int daemonize;
// };
//
// enum OptionType {
// 	no_argument,
// 	required_argument,
// 	optional_argument
// };
