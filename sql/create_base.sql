-- Table: public.builds

-- DROP TABLE IF EXISTS public.builds;

CREATE TABLE IF NOT EXISTS public.builds
(
    id integer NOT NULL DEFAULT nextval('builds_id_seq'::regclass),
    build_name text COLLATE pg_catalog."default" NOT NULL,
    build_config text COLLATE pg_catalog."default" NOT NULL,
    cdn_config text COLLATE pg_catalog."default" NOT NULL,
    product_name text COLLATE pg_catalog."default",
    CONSTRAINT builds_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.builds
    OWNER to postgres;
-- Index: ix_builds_product_name

-- DROP INDEX IF EXISTS public.ix_builds_product_name;

CREATE INDEX IF NOT EXISTS ix_builds_product_name
    ON public.builds USING hash
    (product_name COLLATE pg_catalog."default")
    TABLESPACE pg_default;
