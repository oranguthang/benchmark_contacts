from aiohttp import web
from models import Contact
from db import SessionLocal
from sqlalchemy.future import select
from pydantic import BaseModel


class ContactCreate(BaseModel):
    external_id: int
    phone_number: str


async def create_contact(request):
    data = await request.json()
    contact_data = ContactCreate(**data)

    async with SessionLocal() as session:
        contact = Contact(
            external_id=contact_data.external_id,
            phone_number=contact_data.phone_number
        )
        session.add(contact)
        await session.commit()
        await session.refresh(contact)

        return web.json_response({
            "id": str(contact.id),
            "external_id": contact.external_id,
            "phone_number": contact.phone_number,
            "date_created": contact.date_created.isoformat(),
            "date_updated": contact.date_updated.isoformat()
        }, status=201)


async def get_contacts(request):
    params = request.query
    external_id = params.get("external_id")
    phone_number = params.get("phone_number")
    limit = min(int(params.get("limit", 10000)), 10000)
    offset = int(params.get("offset", 0))

    async with SessionLocal() as session:
        query = select(Contact)

        if external_id:
            query = query.filter(Contact.external_id == int(external_id))
        if phone_number:
            query = query.filter(Contact.phone_number == phone_number)

        result = await session.execute(query.offset(offset).limit(limit))
        contacts = result.scalars().all()

        return web.json_response([
            {
                "id": str(c.id),
                "external_id": c.external_id,
                "phone_number": c.phone_number,
                "date_created": c.date_created.isoformat(),
                "date_updated": c.date_updated.isoformat()
            } for c in contacts
        ])


async def ping(request):
    return web.Response(text="pong")
