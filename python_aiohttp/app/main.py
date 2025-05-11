from aiohttp import web
from routes import create_contact, get_contacts, ping


async def init_app():
    app = web.Application()
    app.router.add_post('/contacts', create_contact)
    app.router.add_get('/contacts', get_contacts)
    app.router.add_get('/ping', ping)

    return app


web.run_app(init_app(), port=8080)
