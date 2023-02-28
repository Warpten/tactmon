PGDMP                           {            tactmon    15.1    15.1                0    0    ENCODING    ENCODING        SET client_encoding = 'UTF8';
                      false                       0    0 
   STDSTRINGS 
   STDSTRINGS     (   SET standard_conforming_strings = 'on';
                      false                       0    0 
   SEARCHPATH 
   SEARCHPATH     8   SELECT pg_catalog.set_config('search_path', '', false);
                      false                       1262    16398    tactmon    DATABASE     �   CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';
    DROP DATABASE tactmon;
                postgres    false                       0    0    DATABASE tactmon    COMMENT     I   COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';
                   postgres    false    3347            �            1259    16608    bound_channels    TABLE     �   CREATE TABLE public.bound_channels (
    id integer NOT NULL,
    guild_id bigint NOT NULL,
    channel_id bigint NOT NULL,
    product_name text NOT NULL
);
 "   DROP TABLE public.bound_channels;
       public         heap    postgres    false            �            1259    16607    bound_channels_id_seq    SEQUENCE     �   CREATE SEQUENCE public.bound_channels_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.bound_channels_id_seq;
       public          postgres    false    219                       0    0    bound_channels_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;
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
       public         heap    postgres    false                       0    0    COLUMN builds.detected_at    COMMENT     �   COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';
          public          postgres    false    215            �            1259    16399    builds_id_seq    SEQUENCE     �   CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 $   DROP SEQUENCE public.builds_id_seq;
       public          postgres    false    215                       0    0    builds_id_seq    SEQUENCE OWNED BY     ?   ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;
          public          postgres    false    214            �            1259    16414    products_id_seq    SEQUENCE     �   CREATE SEQUENCE public.products_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 &   DROP SEQUENCE public.products_id_seq;
       public          postgres    false            �            1259    16415    products    TABLE     �   CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);
    DROP TABLE public.products;
       public         heap    postgres    false    216            r           2604    16611    bound_channels id    DEFAULT     v   ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);
 @   ALTER TABLE public.bound_channels ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    218    219    219            o           2604    16403 	   builds id    DEFAULT     f   ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);
 8   ALTER TABLE public.builds ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    214    215    215                      0    16608    bound_channels 
   TABLE DATA           P   COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM stdin;
    public          postgres    false    219   W       	          0    16400    builds 
   TABLE DATA           m   COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM stdin;
    public          postgres    false    215   t                 0    16415    products 
   TABLE DATA           9   COPY public.products (id, name, sequence_id) FROM stdin;
    public          postgres    false    217   O                  0    0    bound_channels_id_seq    SEQUENCE SET     D   SELECT pg_catalog.setval('public.bound_channels_id_seq', 1, false);
          public          postgres    false    218                       0    0    builds_id_seq    SEQUENCE SET     <   SELECT pg_catalog.setval('public.builds_id_seq', 29, true);
          public          postgres    false    214                       0    0    products_id_seq    SEQUENCE SET     ?   SELECT pg_catalog.setval('public.products_id_seq', 270, true);
          public          postgres    false    216            y           2606    16615 "   bound_channels bound_channels_pkey 
   CONSTRAINT     l   ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);
 L   ALTER TABLE ONLY public.bound_channels DROP CONSTRAINT bound_channels_pkey;
       public            postgres    false    219    219            t           2606    16407    builds builds_pkey 
   CONSTRAINT     P   ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);
 <   ALTER TABLE ONLY public.builds DROP CONSTRAINT builds_pkey;
       public            postgres    false    215            w           2606    16422    products products_pkey 
   CONSTRAINT     T   ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);
 @   ALTER TABLE ONLY public.products DROP CONSTRAINT products_pkey;
       public            postgres    false    217            u           1259    16408    ix_builds_product_name    INDEX     P   CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);
 *   DROP INDEX public.ix_builds_product_name;
       public            postgres    false    215                  x������ � �      	   �  x���QN1���]y<3���=@��K���-**����	�"E�R��7�������^}"�>ޥ����+^_�m���*@�\ ���8*��R,�&�D���9f�4�D���II\�v��	�g�2=l�ꃡg.x��;p�_Zj�C�1��-����i��6<�v���mw&�6?��M�G��;*�;73i�T%4�3#�5��X>����Q�] ;��} K�N�K�^ �%{7�ƒ��c��8l��+Z���M�l�u�L��6�I�Ӊ�^֜�Ko����|曾����:������W(QT�����rB��eA�����Eq���B9��޻��K���/�wU�V5�X��ژS�$���@�'���sKг�p�� В��c	��v9�����R��ji�|�j�
���m��$���;/?�����|_>N�����}��{����1 l��         �  x�m��#��s��]Vj�R=c���Sv�����==�����"�"��쾭��r�1vG�e��w�ͥew�ߖ�Zk1�l���Ѐ�Creٽ���,����^z*��f�=��u+[����Zui-��D�!zcGمHtc���=��^���Ԛ��Q
*���F���(U'�(s��$��D�*Qp��@=;��;QqB�S�Q��OE;��鈢xa��P��O)��z�}L.R��W\�r���D
��@���*Ƣnz��.��n3���Ŷ�.�O���r=�.��]д?�Ȩº%�T���QR���������^��m`ߦ���q)-��m�X2���awߝ�ߔ�X
��h�ZADE�0���]j(�YĈ}y]/�i|)���+�_C{q��T���c�.�/�Kt9�ƅ�rZ^��j����ʻ��0�Xyπ�r5p �.����}���:�"��Ն�]�FƇ�}�?V���ɕH$a�K"�DٕL���+�(�/�JT�pt5�J��:Qc_��)�ᐣ�)��J�2�TW)~
M�R;u��*�S&�J�T���R8E�L�Ԉ�V){J�����zڎ�����k�!���, ״�&V��
zŵ�g��������F��S�g��T��ج��.��튍�#�YkV>((�w׺�;As�/�GƉ����H@����RC��������El�T������-)W��X;9��^� �����^/c��+F���ϼ'�a�U>�%2$9/d�L�e$+d�M>��.I"��lW$��A���R��|%�<�a4��"E�����:5]L�0hb�@�0�j���JE�Q�=tF�'<�� V���G&�Q�eF���Ѻ;W-
}í�ui���XU�u�Y����:H��1����������"fZ
Y�W�2�`RYa�����C`���#�{�	��"D����͉�=����*�iC:hDڐ:�#�_��HZh�t*�̯�����ƅ���	S����g�B���E���m����Ý& +�)�A��IXId�^	��L�iCVRH`��MO�h}ב����Y�͎h��@H0�K�ڑ�H��B#�o��쫐�V�1���+rPR�d�G6��1�QB��!uz���9�]������aa�+��b��+�aВ&�K� ��0���`f�T��U����B�p(�_���%�L���z�Bd
�Fʄ��j�0b�j^~�8��5#wN��`&���`:���LG
о��cw04�f�p04CJ�v���t��d��!#�`>F�FB~�i�`����j�kʑ��*�Bt]Yd㯕��j��[�l�b���d)�鑋��˄���u�	��}�z���ɣxgY�Pa��J�vR ��ne͝��.���}¢�< !�ӽ��<�HO��#3&��$��"��y�D��A�,���=X!K`��RV�Ѓ52�������>(O�w�6�ְ.�i���e����7��	����(W_���Z�ǁ�`��H�h�Zq�	��qz[�l���̖]ꌵ���`��U�����TsT�����gY5��(c�u�0A��z4�L|���,\T�+��SZ�IY�w���8���FB6��H�F� ��dl���C��üD�r���;D�����yw������ڨ�2�V�n�û��纗�qؠ^;������dm��c��1K��ň�EY!��(6E�̮��5��FM��������۝��a�Y%���썣)�<�Kf�s��
2�i�WG\�$�������s}��ߊ��v�D1h����!*�9������y}��]�֪������8�#�+��e��F�8���q�ۈ�k���9�ױ�'�q+ٌîʍl���IEr'�?����%Kd���2��M$d��A^&sm:,�e�V-4�� ����sT�iCZ�l�j�1
�*t�����s�_�xf     