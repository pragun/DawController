################################################################################
#
# python-tabulate
#
################################################################################

PYTHON_TABULATE_VERSION = 0.8.3
PYTHON_TABULATE_SOURCE = tabulate-$(PYTHON_TABULATE_VERSION).tar.gz
PYTHON_TABULATE_SITE = https://files.pythonhosted.org/packages/c2/fd/202954b3f0eb896c53b7b6f07390851b1fd2ca84aa95880d7ae4f434c4ac
PYTHON_TABULATE_SETUP_TYPE = setuptools
PYTHON_TABULATE_LICENSE = MIT
PYTHON_TABULATE_LICENSE_FILES = LICENSE

$(eval $(python-package))
