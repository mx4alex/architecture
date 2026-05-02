import asyncio


async def _register_and_login(service_client, login):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': login,
            'password': 'pass',
            'first_name': 'Perf',
            'last_name': 'Test',
        },
    )
    response = await service_client.post(
        '/api/v1/auth/login',
        json={'login': login, 'password': 'pass'},
    )
    return response.json()['token']


async def test_incomes_get_is_cached(service_client):
    token = await _register_and_login(service_client, 'cache_inc_user')
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 12345.0,
            'date': '2026-04-15',
            'description': 'cached',
        },
        headers=headers,
    )
    assert create_resp.status == 201

    first = await service_client.get('/api/v1/incomes', headers=headers)
    assert first.status == 200
    assert first.headers.get('X-Cache') == 'MISS'

    second = await service_client.get('/api/v1/incomes', headers=headers)
    assert second.status == 200
    assert second.headers.get('X-Cache') == 'HIT'
    assert second.json() == first.json()


async def test_dynamics_is_cached(service_client):
    token = await _register_and_login(service_client, 'cache_dyn_user')
    headers = {'Authorization': f'Bearer {token}'}

    await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 100.0,
            'date': '2026-04-10',
            'description': '',
        },
        headers=headers,
    )

    params = {'from': '2026-04-01', 'to': '2026-04-30'}

    miss_resp = await service_client.get(
        '/api/v1/budget/dynamics', params=params, headers=headers,
    )
    assert miss_resp.status == 200
    assert miss_resp.headers.get('X-Cache') == 'MISS'

    hit_resp = await service_client.get(
        '/api/v1/budget/dynamics', params=params, headers=headers,
    )
    assert hit_resp.status == 200
    assert hit_resp.headers.get('X-Cache') == 'HIT'
    assert hit_resp.json() == miss_resp.json()


async def test_cache_invalidated_on_write(service_client):
    token = await _register_and_login(service_client, 'cache_inv_user')
    headers = {'Authorization': f'Bearer {token}'}

    await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 1.0,
            'date': '2026-04-10',
            'description': '',
        },
        headers=headers,
    )

    miss = await service_client.get('/api/v1/incomes', headers=headers)
    assert miss.headers.get('X-Cache') == 'MISS'
    hit = await service_client.get('/api/v1/incomes', headers=headers)
    assert hit.headers.get('X-Cache') == 'HIT'

    await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Bonus',
            'amount': 2.0,
            'date': '2026-04-11',
            'description': '',
        },
        headers=headers,
    )

    after = await service_client.get('/api/v1/incomes', headers=headers)
    assert after.status == 200
    assert after.headers.get('X-Cache') == 'MISS'
    assert len(after.json()) == 2


async def test_user_search_cache(service_client):
    token = await _register_and_login(service_client, 'cache_srch_user')
    headers = {'Authorization': f'Bearer {token}'}

    miss = await service_client.get(
        '/api/v1/users/search', params={'mask': 'Perf'}, headers=headers,
    )
    assert miss.status == 200
    assert miss.headers.get('X-Cache') == 'MISS'

    hit = await service_client.get(
        '/api/v1/users/search', params={'mask': 'Perf'}, headers=headers,
    )
    assert hit.status == 200
    assert hit.headers.get('X-Cache') == 'HIT'
    assert hit.json() == miss.json()


async def test_user_search_cache_invalidated_on_register(service_client):
    token = await _register_and_login(service_client, 'cache_srch_inv_user')
    headers = {'Authorization': f'Bearer {token}'}

    await service_client.get(
        '/api/v1/users/search', params={'mask': 'Test'}, headers=headers,
    )
    hit = await service_client.get(
        '/api/v1/users/search', params={'mask': 'Test'}, headers=headers,
    )
    assert hit.headers.get('X-Cache') == 'HIT'

    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'inv_search_extra',
            'password': 'p',
            'first_name': 'Extra',
            'last_name': 'Test',
        },
    )

    after = await service_client.get(
        '/api/v1/users/search', params={'mask': 'Test'}, headers=headers,
    )
    assert after.headers.get('X-Cache') == 'MISS'


async def test_login_rate_limit_headers(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'rl_user',
            'password': 'pass',
            'first_name': 'R',
            'last_name': 'L',
        },
    )
    response = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'rl_user', 'password': 'pass'},
    )
    assert response.status == 200
    assert 'X-RateLimit-Limit' in response.headers
    assert 'X-RateLimit-Remaining' in response.headers
    assert 'X-RateLimit-Reset' in response.headers
    assert int(response.headers['X-RateLimit-Limit']) == 10


async def test_login_rate_limit_429(service_client):
    statuses = []
    for i in range(15):
        response = await service_client.post(
            '/api/v1/auth/login',
            json={'login': f'no_such_user_{i}', 'password': 'wrong'},
        )
        statuses.append(response.status)
        if response.status == 429:
            assert response.headers.get('Retry-After') is not None
            assert response.headers.get('X-RateLimit-Remaining') == '0'
            break

    assert 429 in statuses, f'Expected 429 in statuses, got {statuses}'


async def test_register_rate_limit_429(service_client):
    statuses = []
    for i in range(10):
        response = await service_client.post(
            '/api/v1/auth/register',
            json={
                'login': f'rl_register_{i}',
                'password': 'pass',
                'first_name': 'A',
                'last_name': 'B',
            },
        )
        statuses.append(response.status)
        if response.status == 429:
            assert response.headers.get('Retry-After') is not None
            break

    assert 429 in statuses, f'Expected 429 in statuses, got {statuses}'


async def test_dynamics_rate_limit_headers(service_client):
    token = await _register_and_login(service_client, 'rl_dyn_user')
    headers = {'Authorization': f'Bearer {token}'}

    response = await service_client.get(
        '/api/v1/budget/dynamics',
        params={'from': '2026-01-01', 'to': '2026-01-31'},
        headers=headers,
    )
    assert response.status == 200
    assert 'X-RateLimit-Limit' in response.headers
    assert int(response.headers['X-RateLimit-Limit']) == 60
