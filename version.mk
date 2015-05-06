VER_MAJOR = 1
VER_MINOR = 0
VER_PATCH = 0

OS_VER = $(shell (ver=$$(awk '{print $$3}' /etc/redhat-release); echo "CentOS$${ver}"; ))

