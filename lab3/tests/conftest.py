import pathlib

import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def service_source_dir():
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope='session')
def initial_data_path(service_source_dir):
    return [service_source_dir / 'tests' / 'initial_data']


@pytest.fixture(scope='session')
def config_fallback_path(service_source_dir):
    return service_source_dir / 'tests' / 'dynamic_config_fallback.json'


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'budget_service',
        [service_source_dir / 'schemas' / 'postgresql'],
    )
    return pgsql_local_create(list(databases.values()))
