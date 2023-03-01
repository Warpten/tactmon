PGDMP     9                    {            tactmon    15.1    15.1                0    0    ENCODING    ENCODING        SET client_encoding = 'UTF8';
                      false                       0    0 
   STDSTRINGS 
   STDSTRINGS     (   SET standard_conforming_strings = 'on';
                      false                       0    0 
   SEARCHPATH 
   SEARCHPATH     8   SELECT pg_catalog.set_config('search_path', '', false);
                      false                       1262    16398    tactmon    DATABASE     �   CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';
    DROP DATABASE tactmon;
                postgres    false                       0    0    DATABASE tactmon    COMMENT     I   COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';
                   postgres    false    3348                        2615    2200    public    SCHEMA        CREATE SCHEMA public;
    DROP SCHEMA public;
                pg_database_owner    false                       0    0    SCHEMA public    COMMENT     6   COMMENT ON SCHEMA public IS 'standard public schema';
                   pg_database_owner    false    4            �            1259    16608    bound_channels    TABLE     �   CREATE TABLE public.bound_channels (
    id integer NOT NULL,
    guild_id bigint NOT NULL,
    channel_id bigint NOT NULL,
    product_name text NOT NULL
);
 "   DROP TABLE public.bound_channels;
       public         heap    postgres    false    4            �            1259    16607    bound_channels_id_seq    SEQUENCE     �   CREATE SEQUENCE public.bound_channels_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.bound_channels_id_seq;
       public          postgres    false    4    219                       0    0    bound_channels_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;
          public          postgres    false    218            �            1259    16400    builds    TABLE     �   CREATE TABLE public.builds (
    id integer NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    product_name text,
    detected_at bigint DEFAULT 0 NOT NULL,
    region text NOT NULL
);
    DROP TABLE public.builds;
       public         heap    postgres    false    4                       0    0    COLUMN builds.detected_at    COMMENT     �   COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';
          public          postgres    false    215            �            1259    16399    builds_id_seq    SEQUENCE     �   CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 $   DROP SEQUENCE public.builds_id_seq;
       public          postgres    false    4    215                       0    0    builds_id_seq    SEQUENCE OWNED BY     ?   ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;
          public          postgres    false    214            �            1259    16414    products_id_seq    SEQUENCE     �   CREATE SEQUENCE public.products_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 &   DROP SEQUENCE public.products_id_seq;
       public          postgres    false    4            �            1259    16415    products    TABLE     �   CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);
    DROP TABLE public.products;
       public         heap    postgres    false    216    4            r           2604    16611    bound_channels id    DEFAULT     v   ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);
 @   ALTER TABLE public.bound_channels ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    218    219    219            o           2604    16403 	   builds id    DEFAULT     f   ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);
 8   ALTER TABLE public.builds ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    215    214    215                      0    16608    bound_channels 
   TABLE DATA           P   COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM stdin;
    public          postgres    false    219   �       
          0    16400    builds 
   TABLE DATA           m   COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM stdin;
    public          postgres    false    215                    0    16415    products 
   TABLE DATA           9   COPY public.products (id, name, sequence_id) FROM stdin;
    public          postgres    false    217   !                  0    0    bound_channels_id_seq    SEQUENCE SET     C   SELECT pg_catalog.setval('public.bound_channels_id_seq', 1, true);
          public          postgres    false    218                       0    0    builds_id_seq    SEQUENCE SET     ;   SELECT pg_catalog.setval('public.builds_id_seq', 1, true);
          public          postgres    false    214                       0    0    products_id_seq    SEQUENCE SET     =   SELECT pg_catalog.setval('public.products_id_seq', 1, true);
          public          postgres    false    216            z           2606    16615 "   bound_channels bound_channels_pkey 
   CONSTRAINT     l   ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);
 L   ALTER TABLE ONLY public.bound_channels DROP CONSTRAINT bound_channels_pkey;
       public            postgres    false    219    219            t           2606    16407    builds builds_pkey 
   CONSTRAINT     P   ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);
 <   ALTER TABLE ONLY public.builds DROP CONSTRAINT builds_pkey;
       public            postgres    false    215            x           2606    16422    products products_pkey 
   CONSTRAINT     T   ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);
 @   ALTER TABLE ONLY public.products DROP CONSTRAINT products_pkey;
       public            postgres    false    217            u           1259    16408    ix_builds_product_name    INDEX     P   CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);
 *   DROP INDEX public.ix_builds_product_name;
       public            postgres    false    215            v           1259    17063    ix_products_name    INDEX     L   CREATE UNIQUE INDEX ix_products_name ON public.products USING btree (name);
 $   DROP INDEX public.ix_products_name;
       public            postgres    false    217                  x������ � �      
      x������ � �            x������ � �     