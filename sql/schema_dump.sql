--
-- PostgreSQL database dump
--

-- Dumped from database version 15.1
-- Dumped by pg_dump version 15.1

-- Started on 2023-03-10 00:01:54

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 219 (class 1259 OID 16608)
-- Name: bound_channels; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.bound_channels (
    id integer NOT NULL,
    guild_id bigint NOT NULL,
    channel_id bigint NOT NULL,
    product_name text NOT NULL
);


ALTER TABLE public.bound_channels OWNER TO postgres;

--
-- TOC entry 218 (class 1259 OID 16607)
-- Name: bound_channels_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.bound_channels_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.bound_channels_id_seq OWNER TO postgres;

--
-- TOC entry 3350 (class 0 OID 0)
-- Dependencies: 218
-- Name: bound_channels_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;


--
-- TOC entry 215 (class 1259 OID 16400)
-- Name: builds; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.builds (
    id integer NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    product_name text,
    detected_at bigint DEFAULT 0 NOT NULL,
    region text NOT NULL
);


ALTER TABLE public.builds OWNER TO postgres;

--
-- TOC entry 3351 (class 0 OID 0)
-- Dependencies: 215
-- Name: COLUMN builds.detected_at; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';


--
-- TOC entry 214 (class 1259 OID 16399)
-- Name: builds_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.builds_id_seq OWNER TO postgres;

--
-- TOC entry 3352 (class 0 OID 0)
-- Dependencies: 214
-- Name: builds_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;


--
-- TOC entry 216 (class 1259 OID 16414)
-- Name: products_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.products_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.products_id_seq OWNER TO postgres;

--
-- TOC entry 217 (class 1259 OID 16415)
-- Name: products; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);


ALTER TABLE public.products OWNER TO postgres;

--
-- TOC entry 221 (class 1259 OID 17169)
-- Name: tracked_files; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.tracked_files (
    id integer NOT NULL,
    product_name text,
    file_path text,
    display_name text
);


ALTER TABLE public.tracked_files OWNER TO postgres;

--
-- TOC entry 220 (class 1259 OID 17168)
-- Name: tracked_file_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.tracked_file_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.tracked_file_id_seq OWNER TO postgres;

--
-- TOC entry 3353 (class 0 OID 0)
-- Dependencies: 220
-- Name: tracked_file_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.tracked_file_id_seq OWNED BY public.tracked_files.id;


--
-- TOC entry 3191 (class 2604 OID 16611)
-- Name: bound_channels id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);


--
-- TOC entry 3188 (class 2604 OID 16403)
-- Name: builds id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);


--
-- TOC entry 3192 (class 2604 OID 17172)
-- Name: tracked_files id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.tracked_files ALTER COLUMN id SET DEFAULT nextval('public.tracked_file_id_seq'::regclass);


--
-- TOC entry 3200 (class 2606 OID 16615)
-- Name: bound_channels bound_channels_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);


--
-- TOC entry 3194 (class 2606 OID 16407)
-- Name: builds builds_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);


--
-- TOC entry 3202 (class 2606 OID 17176)
-- Name: tracked_files pk_tracked_file_product_name_file_path; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.tracked_files
    ADD CONSTRAINT pk_tracked_file_product_name_file_path UNIQUE (product_name, file_path);


--
-- TOC entry 3198 (class 2606 OID 16422)
-- Name: products products_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);


--
-- TOC entry 3195 (class 1259 OID 16408)
-- Name: ix_builds_product_name; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);


--
-- TOC entry 3196 (class 1259 OID 17063)
-- Name: ix_products_name; Type: INDEX; Schema: public; Owner: postgres
--

CREATE UNIQUE INDEX ix_products_name ON public.products USING btree (name);


-- Completed on 2023-03-10 00:01:54

--
-- PostgreSQL database dump complete
--

