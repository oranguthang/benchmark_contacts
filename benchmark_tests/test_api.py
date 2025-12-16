import os

from unittest.mock import ANY
from uuid import UUID
import pytest
import httpx
from pytest import param
from dateutil import parser


@pytest.fixture
def base_url(request):
    return os.environ.get('BASE_URL')

def is_valid_date(date_string):
    try:
        return parser.isoparse(date_string)
    except (ValueError, TypeError):
        pytest.fail(f"Invalid date format: {date_string}")


def check_contact(contact):
    assert contact.keys() == {"id", "external_id", "phone_number", "date_created", "date_updated"}
    assert isinstance(contact["id"], str) and bool(UUID(contact["id"]))
    assert isinstance(contact["external_id"], int)
    assert isinstance(contact["phone_number"], str)
    assert isinstance(contact["date_created"], str) and is_valid_date(contact["date_created"])
    assert isinstance(contact["date_updated"], str) and is_valid_date(contact["date_updated"])


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
def test_create_contact(base_url, contact):
    response = httpx.post(f"{base_url}/contacts", json=contact)
    assert response.status_code == 201
    data = response.json()

    # Check that all fields are present and correct
    expected_response = {
        "id": ANY,
        "external_id": contact["external_id"],
        "phone_number": contact["phone_number"],
        "date_created": ANY,
        "date_updated": ANY,
    }
    check_contact(data)
    assert data == expected_response


@pytest.mark.parametrize("params, expected_count", [
    ({"external_id": "100"}, 1),
    ({"phone_number": "555-0101"}, 1),
    ({"limit": "5"}, 5),
    ({"offset": "1"}, 9),
    ({"external_id": "101", "phone_number": "555-0101"}, 1),
])
def test_get_contacts_with_filters(base_url, params, expected_count):
    response = httpx.get(f"{base_url}/contacts", params=params)
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
    assert len(data) == expected_count

    for contact in data:
        check_contact(contact)


def test_get_all_contacts(base_url):
    response = httpx.get(f"{base_url}/contacts")
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
    assert len(data) == 10  # We created 10 contacts

    for contact in data:
        check_contact(contact)


def test_ping(base_url):
    response = httpx.get(f"{base_url}/ping")
    assert response.status_code == 200
    data = response.text
    assert data == "pong"
