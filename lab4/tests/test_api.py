async def test_ping(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200


async def test_register_success(service_client):
    response = await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'test_user',
            'password': 'test_pass',
            'first_name': 'Test',
            'last_name': 'User',
        },
    )
    assert response.status == 201
    data = response.json()
    assert 'id' in data
    assert data['login'] == 'test_user'
    assert data['first_name'] == 'Test'
    assert data['last_name'] == 'User'


async def test_register_duplicate(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'dup_user',
            'password': 'pass',
            'first_name': 'A',
            'last_name': 'B',
        },
    )
    response = await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'dup_user',
            'password': 'pass2',
            'first_name': 'C',
            'last_name': 'D',
        },
    )
    assert response.status == 409


async def test_register_missing_fields(service_client):
    response = await service_client.post(
        '/api/v1/auth/register',
        json={'login': 'incomplete'},
    )
    assert response.status == 400


async def test_login_success(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'login_user',
            'password': 'secret',
            'first_name': 'L',
            'last_name': 'U',
        },
    )
    response = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'login_user', 'password': 'secret'},
    )
    assert response.status == 200
    data = response.json()
    assert 'token' in data
    assert len(data['token']) > 0


async def test_login_wrong_password(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'wrong_pass_user',
            'password': 'correct',
            'first_name': 'W',
            'last_name': 'P',
        },
    )
    response = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'wrong_pass_user', 'password': 'incorrect'},
    )
    assert response.status == 401


async def test_protected_endpoint_no_auth(service_client):
    response = await service_client.get('/api/v1/incomes')
    assert response.status == 401


async def test_user_search_by_login(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'search_user',
            'password': 'pass',
            'first_name': 'Search',
            'last_name': 'User',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'search_user', 'password': 'pass'},
    )
    token = login_resp.json()['token']

    response = await service_client.get(
        '/api/v1/users/search',
        params={'login': 'search_user'},
        headers={'Authorization': f'Bearer {token}'},
    )
    assert response.status == 200
    data = response.json()
    assert len(data) == 1
    assert data[0]['login'] == 'search_user'


async def test_user_search_by_name(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'name_search',
            'password': 'pass',
            'first_name': 'Ivan',
            'last_name': 'Petrov',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'name_search', 'password': 'pass'},
    )
    token = login_resp.json()['token']

    response = await service_client.get(
        '/api/v1/users/search',
        params={'name': 'Ivan'},
        headers={'Authorization': f'Bearer {token}'},
    )
    assert response.status == 200
    data = response.json()
    assert any(u['first_name'] == 'Ivan' for u in data)


async def test_income_crud(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'income_user',
            'password': 'pass',
            'first_name': 'I',
            'last_name': 'U',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'income_user', 'password': 'pass'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 150000.0,
            'date': '2026-01-15',
            'description': 'Monthly salary',
        },
        headers=headers,
    )
    assert create_resp.status == 201
    income = create_resp.json()
    assert income['category'] == 'Salary'
    assert income['amount'] == 150000.0

    list_resp = await service_client.get(
        '/api/v1/incomes', headers=headers,
    )
    assert list_resp.status == 200
    incomes = list_resp.json()
    assert len(incomes) >= 1


async def test_expense_crud(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'expense_user',
            'password': 'pass',
            'first_name': 'E',
            'last_name': 'U',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'expense_user', 'password': 'pass'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/expenses',
        json={
            'category': 'Food',
            'amount': 15000.0,
            'date': '2026-01-10',
            'description': 'Weekly groceries',
        },
        headers=headers,
    )
    assert create_resp.status == 201
    expense = create_resp.json()
    assert expense['category'] == 'Food'
    assert expense['amount'] == 15000.0

    list_resp = await service_client.get(
        '/api/v1/expenses', headers=headers,
    )
    assert list_resp.status == 200
    expenses = list_resp.json()
    assert len(expenses) >= 1


async def test_budget_dynamics(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'budget_user',
            'password': 'pass',
            'first_name': 'B',
            'last_name': 'U',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'budget_user', 'password': 'pass'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 100000.0,
            'date': '2026-01-15',
            'description': 'Salary',
        },
        headers=headers,
    )
    await service_client.post(
        '/api/v1/expenses',
        json={
            'category': 'Rent',
            'amount': 30000.0,
            'date': '2026-01-05',
            'description': 'Apartment rent',
        },
        headers=headers,
    )
    await service_client.post(
        '/api/v1/expenses',
        json={
            'category': 'Food',
            'amount': 10000.0,
            'date': '2026-01-15',
            'description': 'Food',
        },
        headers=headers,
    )

    response = await service_client.get(
        '/api/v1/budget/dynamics',
        params={'from': '2026-01-01', 'to': '2026-01-31'},
        headers=headers,
    )
    assert response.status == 200
    data = response.json()
    assert data['total_income'] == 100000.0
    assert data['total_expense'] == 40000.0
    assert data['balance'] == 60000.0
    assert data['period_from'] == '2026-01-01'
    assert data['period_to'] == '2026-01-31'
    assert len(data['daily']) >= 2


async def test_budget_dynamics_missing_params(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'dyn_user',
            'password': 'p',
            'first_name': 'D',
            'last_name': 'U',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'dyn_user', 'password': 'p'},
    )
    token = login_resp.json()['token']

    response = await service_client.get(
        '/api/v1/budget/dynamics',
        headers={'Authorization': f'Bearer {token}'},
    )
    assert response.status == 400


async def test_update_income(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'upd_inc_user',
            'password': 'p',
            'first_name': 'U',
            'last_name': 'I',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'upd_inc_user', 'password': 'p'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Salary',
            'amount': 100000.0,
            'date': '2026-03-01',
            'description': 'Old',
        },
        headers=headers,
    )
    income_id = create_resp.json()['id']

    put_resp = await service_client.put(
        f'/api/v1/incomes/{income_id}',
        json={
            'category': 'Salary Updated',
            'amount': 120000.0,
            'date': '2026-03-01',
            'description': 'Raised',
        },
        headers=headers,
    )
    assert put_resp.status == 200
    data = put_resp.json()
    assert data['category'] == 'Salary Updated'
    assert data['amount'] == 120000.0


async def test_update_income_not_found(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'upd_inc_nf',
            'password': 'p',
            'first_name': 'U',
            'last_name': 'N',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'upd_inc_nf', 'password': 'p'},
    )
    token = login_resp.json()['token']

    put_resp = await service_client.put(
        '/api/v1/incomes/999999',
        json={
            'category': 'X',
            'amount': 1.0,
            'date': '2026-01-01',
            'description': 'X',
        },
        headers={'Authorization': f'Bearer {token}'},
    )
    assert put_resp.status == 404


async def test_update_expense(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'upd_exp_user',
            'password': 'p',
            'first_name': 'U',
            'last_name': 'E',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'upd_exp_user', 'password': 'p'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/expenses',
        json={
            'category': 'Rent',
            'amount': 40000.0,
            'date': '2026-03-01',
            'description': 'Old rent',
        },
        headers=headers,
    )
    expense_id = create_resp.json()['id']

    put_resp = await service_client.put(
        f'/api/v1/expenses/{expense_id}',
        json={
            'category': 'Rent Updated',
            'amount': 45000.0,
            'date': '2026-03-01',
            'description': 'New rent',
        },
        headers=headers,
    )
    assert put_resp.status == 200
    data = put_resp.json()
    assert data['category'] == 'Rent Updated'
    assert data['amount'] == 45000.0


async def test_delete_income(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'del_inc_user',
            'password': 'p',
            'first_name': 'D',
            'last_name': 'I',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'del_inc_user', 'password': 'p'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/incomes',
        json={
            'category': 'Bonus',
            'amount': 5000.0,
            'date': '2026-02-01',
            'description': 'Quarterly bonus',
        },
        headers=headers,
    )
    assert create_resp.status == 201
    income_id = create_resp.json()['id']

    del_resp = await service_client.delete(
        f'/api/v1/incomes/{income_id}',
        headers=headers,
    )
    assert del_resp.status == 200
    assert del_resp.json()['status'] == 'deleted'

    list_resp = await service_client.get('/api/v1/incomes', headers=headers)
    assert all(i['id'] != income_id for i in list_resp.json())


async def test_delete_income_not_found(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'del_inc_nf',
            'password': 'p',
            'first_name': 'D',
            'last_name': 'N',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'del_inc_nf', 'password': 'p'},
    )
    token = login_resp.json()['token']

    del_resp = await service_client.delete(
        '/api/v1/incomes/999999',
        headers={'Authorization': f'Bearer {token}'},
    )
    assert del_resp.status == 404


async def test_delete_expense(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'del_exp_user',
            'password': 'p',
            'first_name': 'D',
            'last_name': 'E',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'del_exp_user', 'password': 'p'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    create_resp = await service_client.post(
        '/api/v1/expenses',
        json={
            'category': 'Transport',
            'amount': 2000.0,
            'date': '2026-02-05',
            'description': 'Monthly pass',
        },
        headers=headers,
    )
    assert create_resp.status == 201
    expense_id = create_resp.json()['id']

    del_resp = await service_client.delete(
        f'/api/v1/expenses/{expense_id}',
        headers=headers,
    )
    assert del_resp.status == 200
    assert del_resp.json()['status'] == 'deleted'

    list_resp = await service_client.get('/api/v1/expenses', headers=headers)
    assert all(e['id'] != expense_id for e in list_resp.json())


async def test_create_income_invalid_data(service_client):
    await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': 'invalid_inc_user',
            'password': 'p',
            'first_name': 'I',
            'last_name': 'U',
        },
    )
    login_resp = await service_client.post(
        '/api/v1/auth/login',
        json={'login': 'invalid_inc_user', 'password': 'p'},
    )
    token = login_resp.json()['token']
    headers = {'Authorization': f'Bearer {token}'}

    response = await service_client.post(
        '/api/v1/incomes',
        json={'category': '', 'amount': -100, 'date': ''},
        headers=headers,
    )
    assert response.status == 400
