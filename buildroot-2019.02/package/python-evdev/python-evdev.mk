################################################################################
#
# python-evdev
#
################################################################################

PYTHON_EVDEV_VERSION = 1.1.2
PYTHON_EVDEV_SOURCE = evdev-$(PYTHON_EVDEV_VERSION).tar.gz
PYTHON_EVDEV_SITE = https://files.pythonhosted.org/packages/7e/53/374b82dd2ccec240b7388c65075391147524255466651a14340615aabb5f
PYTHON_EVDEV_SETUP_TYPE = setuptools
PYTHON_EVDEV_LICENSE = BSD
PYTHON_EVDEV_LICENSE_FILES = LICENSE
PYTHON_EVDEV_DEPENDENCIES = host-python-cffi

$(eval $(python-package))

