CREATE TABLE IF NOT EXISTS public.builds
(
    id serial NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    PRIMARY KEY (id)
);
