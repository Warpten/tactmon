PGDMP     1                    {            tactmon    15.1    15.1     �           0    0    ENCODING    ENCODING        SET client_encoding = 'UTF8';
                      false            �           0    0 
   STDSTRINGS 
   STDSTRINGS     (   SET standard_conforming_strings = 'on';
                      false            �           0    0 
   SEARCHPATH 
   SEARCHPATH     8   SELECT pg_catalog.set_config('search_path', '', false);
                      false            �           1262    16398    tactmon    DATABASE     �   CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';
    DROP DATABASE tactmon;
                postgres    false            �           0    0    DATABASE tactmon    COMMENT     I   COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';
                   postgres    false    3325            �            1259    16400    builds    TABLE     �   CREATE TABLE public.builds (
    id integer NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    product_name text,
    detected_at bigint DEFAULT 0 NOT NULL
);
    DROP TABLE public.builds;
       public         heap    postgres    false            �           0    0    COLUMN builds.detected_at    COMMENT     �   COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';
          public          postgres    false    215            �            1259    16399    builds_id_seq    SEQUENCE     �   CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 $   DROP SEQUENCE public.builds_id_seq;
       public          postgres    false    215                        0    0    builds_id_seq    SEQUENCE OWNED BY     ?   ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;
          public          postgres    false    214            e           2604    16403 	   builds id    DEFAULT     f   ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);
 8   ALTER TABLE public.builds ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    215    214    215            h           2606    16407    builds builds_pkey 
   CONSTRAINT     P   ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);
 <   ALTER TABLE ONLY public.builds DROP CONSTRAINT builds_pkey;
       public            postgres    false    215            i           1259    16408    ix_builds_product_name    INDEX     P   CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);
 *   DROP INDEX public.ix_builds_product_name;
       public            postgres    false    215           