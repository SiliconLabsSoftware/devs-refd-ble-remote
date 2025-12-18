import sys
from log import initialize_logging, get_logger
from config import Config
config = Config()
initialize_logging(config)
from app import App, create_app_dependencies

logger = get_logger(__name__)

if __name__ == "__main__":
    """Application entry point. Creates dependencies and runs the application."""
    try:
        deps = create_app_dependencies()
        logger.info("Dependencies created successfully")

        app = App(deps)
        logger.info("Application initialized successfully")

        logger.info("Starting application...")
        app.run()
    except RuntimeError as e:
        logger.error("Application startup failed: %s", e)
        sys.exit(1)
    except Exception as e:
        logger.error("Unexpected error during application startup: %s", e, exc_info=True)
        sys.exit(1)
