import pathlib

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def service_source_dir():
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope='session')
def config_fallback_path():
    return None
