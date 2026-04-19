import pathlib

import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.mongo',
    'pytest_userver.plugins.postgresql',
]

MONGO_COLLECTIONS = {
    'planned_incomes': {
        'settings': {
            'collection': 'planned_incomes',
            'connection': 'admin',
            'database': 'budget_db',
        },
        'indexes': [],
    },
    'planned_expenses': {
        'settings': {
            'collection': 'planned_expenses',
            'connection': 'admin',
            'database': 'budget_db',
        },
        'indexes': [],
    },
    'counters': {
        'settings': {
            'collection': 'counters',
            'connection': 'admin',
            'database': 'budget_db',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture(scope='session')
def userver_mongo_config(mongo_connection_info):
    mongo_uri = (
        f'mongodb://{mongo_connection_info.host}:'
        f'{mongo_connection_info.port}/budget_db'
    )

    def patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        mongo_component = components['mongo-db']
        mongo_component['dbconnection'] = mongo_uri
        mongo_component.pop('dbalias', None)
        mongo_component['conn_timeout'] = '30s'
        mongo_component['so_timeout'] = '30s'
        config_vars['mongo-url'] = mongo_uri

    return patch_config

@pytest.fixture(scope='session')
def userver_pg_config(pgsql_local):
    uri = list(pgsql_local.values())[0].get_uri()

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        pg_component = components.get('postgres-db')
        if pg_component is not None:
            pg_component['dbconnection'] = uri
            pg_component.pop('dbalias', None)
        config_vars['db-url'] = uri

    return _patch_config


@pytest.fixture(scope='session')
def service_source_dir():
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope='session')
def config_fallback_path(service_source_dir):
    return service_source_dir / 'tests' / 'dynamic_config_fallback.json'


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    schema_path = service_source_dir / 'db' / 'postgres' / 'schema.sql'
    db = discover.PgShardedDatabase(
        service_name='budget_service',
        dbname='budget_db',
        shards=[
            discover.PgShard(
                shard_id=0,
                pretty_name='budget_db',
                dbname='budget_service_budget_db',
                files=[schema_path],
                migrations=[],
            ),
        ],
    )
    return pgsql_local_create([db])
