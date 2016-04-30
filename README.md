What is Rampage?
================
Rampage is a websocket server framework implemented in C, layered on top of
libwebsockets. Rampage attempts to increase the level of abstraction offered by
libwebsockets by managing the callback loop so that the implementer doesn't have
to do things like assemble and free messages and manually wire user sessions
together to allow them to communicate with each other. Rampage will also handle
things like performing authentication (with open id, or sessions) and serving
out static assets given a static asset path. The framework attempts to be as
simple as possible and easy to read and modify.

Development
===========
Rampage is still in the very early stages of development and is not yet ready
for production use or testing.

License
=======
This software is released under the MIT license.
