import pytest
import httpx
from pytest import param
from _pytest.config import Config

BASE_URL = None


def pytest_addoption(parser):
    parser.addoption(
        "--base-url",
        action="store",
        default="http://target_service:8080",
        help="Base URL for the API service"
    )


def pytest_configure(config: Config):
    global BASE_URL
    BASE_URL = config.getoption("--base-url")


def check_contact(contact):
    assert contact.keys() == {"id", "external_id", "phone_number", "date_created", "date_updated"}
    assert isinstance(contact["id"], int)
    assert isinstance(contact["external_id"], int)
    assert isinstance(contact["phone_number"], str)
    assert isinstance(contact["date_created"], str)
    assert isinstance(contact["date_updated"], str)


@pytest.mark.parametrize("contact", [
    param({"external_id": 100, "phone_number": "555-0100"}, id="contact-100"),
    param({"external_id": 101, "phone_number": "555-0101"}, id="contact-101"),
    param({"external_id": 102, "phone_number": "555-0102"}, id="contact-102"),
    param({"external_id": 103, "phone_number": "555-0103"}, id="contact-103"),
    param({"external_id": 104, "phone_number": "555-0104"}, id="contact-104"),
    param({"external_id": 105, "phone_number": "555-0105"}, id="contact-105"),
    param({"external_id": 106, "phone_number": "555-0106"}, id="contact-106"),
    param({"external_id": 107, "phone_number": "555-0107"}, id="contact-107"),
    param({"external_id": 108, "phone_number": "555-0108"}, id="contact-108"),
    param({"external_id": 109, "phone_number": "555-0109"}, id="contact-109"),
])
def test_create_contact(contact):
    response = httpx.post(f"{BASE_URL}/contacts", json=contact)
    assert response.status_code == 201
    data = response.json()

    # Проверка что все поля есть и корректные
    expected_response = {
        "id": pytest.ANY,
        "external_id": contact["external_id"],
        "phone_number": contact["phone_number"],
        "date_created": pytest.ANY,
        "date_updated": pytest.ANY,
    }
    assert data == expected_response


@pytest.mark.parametrize("params, expected_count", [
    ({"external_id": "100"}, 1),
    ({"phone_number": "555-0101"}, 1),
    ({"limit": "5"}, 5),
    ({"offset": "1"}, 9),
    ({"external_id": "101", "phone_number": "555-0101"}, 1),
])
def test_get_contacts_with_filters(params, expected_count):
    response = httpx.get(f"{BASE_URL}/contacts", params=params)
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
    assert len(data) == expected_count

    for contact in data:
        check_contact(contact)


def test_get_all_contacts():
    response = httpx.get(f"{BASE_URL}/contacts")
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
    assert len(data) == 10  # Мы создали 10 контактов

    for contact in data:
        check_contact(contact)


def test_ping():
    response = httpx.get(f"{BASE_URL}/ping")
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, str)
    assert data == "pong"
