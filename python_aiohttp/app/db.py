from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession
from sqlalchemy.orm import sessionmaker, declarative_base
import os

cpu_cores = int(os.getenv("CPU_CORES", 4))  # Default value is 4 if the variable is not set
pool_size = cpu_cores * 4

DATABASE_URL = os.getenv("DATABASE_URL").replace("postgres://", "postgresql+asyncpg://")

engine = create_async_engine(DATABASE_URL, echo=False, future=True, pool_size=pool_size, max_overflow=0)
SessionLocal = sessionmaker(engine, expire_on_commit=False, class_=AsyncSession)
Base = declarative_base()
