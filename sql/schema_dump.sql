toc.dat                                                                                             0000600 0004000 0002000 00000026442 14414645602 0014455 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        PGDMP           "        
        {           tactmon    15.1    15.1 *    '           0    0    ENCODING    ENCODING        SET client_encoding = 'UTF8';
                      false         (           0    0 
   STDSTRINGS 
   STDSTRINGS     (   SET standard_conforming_strings = 'on';
                      false         )           0    0 
   SEARCHPATH 
   SEARCHPATH     8   SELECT pg_catalog.set_config('search_path', '', false);
                      false         *           1262    16398    tactmon    DATABASE     �   CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';
    DROP DATABASE tactmon;
                postgres    false         +           0    0    DATABASE tactmon    COMMENT     I   COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';
                   postgres    false    3370         �            1259    16608    bound_channels    TABLE     �   CREATE TABLE public.bound_channels (
    id integer NOT NULL,
    guild_id bigint NOT NULL,
    channel_id bigint NOT NULL,
    product_name text NOT NULL
);
 "   DROP TABLE public.bound_channels;
       public         heap    postgres    false         �            1259    16607    bound_channels_id_seq    SEQUENCE     �   CREATE SEQUENCE public.bound_channels_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.bound_channels_id_seq;
       public          postgres    false    219         ,           0    0    bound_channels_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;
          public          postgres    false    218         �            1259    16400    builds    TABLE     �   CREATE TABLE public.builds (
    id integer NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    product_name text,
    detected_at bigint DEFAULT 0 NOT NULL,
    region text NOT NULL
);
    DROP TABLE public.builds;
       public         heap    postgres    false         -           0    0    COLUMN builds.detected_at    COMMENT     �   COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';
          public          postgres    false    215         �            1259    16399    builds_id_seq    SEQUENCE     �   CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 $   DROP SEQUENCE public.builds_id_seq;
       public          postgres    false    215         .           0    0    builds_id_seq    SEQUENCE OWNED BY     ?   ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;
          public          postgres    false    214         �            1259    17178    command_states    TABLE     �   CREATE TABLE public.command_states (
    id integer NOT NULL,
    name text NOT NULL,
    hash bigint NOT NULL,
    version integer NOT NULL
);
 "   DROP TABLE public.command_states;
       public         heap    postgres    false         �            1259    17177    command_states_id_seq    SEQUENCE     �   CREATE SEQUENCE public.command_states_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.command_states_id_seq;
       public          postgres    false    223         /           0    0    command_states_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.command_states_id_seq OWNED BY public.command_states.id;
          public          postgres    false    222         �            1259    16414    products_id_seq    SEQUENCE     �   CREATE SEQUENCE public.products_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 &   DROP SEQUENCE public.products_id_seq;
       public          postgres    false         �            1259    16415    products    TABLE     �   CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);
    DROP TABLE public.products;
       public         heap    postgres    false    216         �            1259    17169    tracked_files    TABLE     �   CREATE TABLE public.tracked_files (
    id integer NOT NULL,
    product_name text,
    file_path text,
    display_name text
);
 !   DROP TABLE public.tracked_files;
       public         heap    postgres    false         �            1259    17168    tracked_file_id_seq    SEQUENCE     �   CREATE SEQUENCE public.tracked_file_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 *   DROP SEQUENCE public.tracked_file_id_seq;
       public          postgres    false    221         0           0    0    tracked_file_id_seq    SEQUENCE OWNED BY     L   ALTER SEQUENCE public.tracked_file_id_seq OWNED BY public.tracked_files.id;
          public          postgres    false    220         |           2604    16611    bound_channels id    DEFAULT     v   ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);
 @   ALTER TABLE public.bound_channels ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    218    219    219         y           2604    16403 	   builds id    DEFAULT     f   ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);
 8   ALTER TABLE public.builds ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    214    215    215         ~           2604    17181    command_states id    DEFAULT     v   ALTER TABLE ONLY public.command_states ALTER COLUMN id SET DEFAULT nextval('public.command_states_id_seq'::regclass);
 @   ALTER TABLE public.command_states ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    223    222    223         }           2604    17172    tracked_files id    DEFAULT     s   ALTER TABLE ONLY public.tracked_files ALTER COLUMN id SET DEFAULT nextval('public.tracked_file_id_seq'::regclass);
 ?   ALTER TABLE public.tracked_files ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    221    220    221                    0    16608    bound_channels 
   TABLE DATA           P   COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM stdin;
    public          postgres    false    219       3360.dat           0    16400    builds 
   TABLE DATA           m   COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM stdin;
    public          postgres    false    215       3356.dat $          0    17178    command_states 
   TABLE DATA           A   COPY public.command_states (id, name, hash, version) FROM stdin;
    public          postgres    false    223       3364.dat           0    16415    products 
   TABLE DATA           9   COPY public.products (id, name, sequence_id) FROM stdin;
    public          postgres    false    217       3358.dat "          0    17169    tracked_files 
   TABLE DATA           R   COPY public.tracked_files (id, product_name, file_path, display_name) FROM stdin;
    public          postgres    false    221       3362.dat 1           0    0    bound_channels_id_seq    SEQUENCE SET     C   SELECT pg_catalog.setval('public.bound_channels_id_seq', 1, true);
          public          postgres    false    218         2           0    0    builds_id_seq    SEQUENCE SET     <   SELECT pg_catalog.setval('public.builds_id_seq', 35, true);
          public          postgres    false    214         3           0    0    command_states_id_seq    SEQUENCE SET     D   SELECT pg_catalog.setval('public.command_states_id_seq', 1, false);
          public          postgres    false    222         4           0    0    products_id_seq    SEQUENCE SET     ?   SELECT pg_catalog.setval('public.products_id_seq', 278, true);
          public          postgres    false    216         5           0    0    tracked_file_id_seq    SEQUENCE SET     A   SELECT pg_catalog.setval('public.tracked_file_id_seq', 2, true);
          public          postgres    false    220         �           2606    16615 "   bound_channels bound_channels_pkey 
   CONSTRAINT     l   ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);
 L   ALTER TABLE ONLY public.bound_channels DROP CONSTRAINT bound_channels_pkey;
       public            postgres    false    219    219         �           2606    16407    builds builds_pkey 
   CONSTRAINT     P   ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);
 <   ALTER TABLE ONLY public.builds DROP CONSTRAINT builds_pkey;
       public            postgres    false    215         �           2606    17187 &   command_states command_state_uniq_name 
   CONSTRAINT     a   ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_state_uniq_name UNIQUE (name);
 P   ALTER TABLE ONLY public.command_states DROP CONSTRAINT command_state_uniq_name;
       public            postgres    false    223         �           2606    17185 "   command_states command_states_pkey 
   CONSTRAINT     `   ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_states_pkey PRIMARY KEY (id);
 L   ALTER TABLE ONLY public.command_states DROP CONSTRAINT command_states_pkey;
       public            postgres    false    223         �           2606    17176 4   tracked_files pk_tracked_file_product_name_file_path 
   CONSTRAINT     �   ALTER TABLE ONLY public.tracked_files
    ADD CONSTRAINT pk_tracked_file_product_name_file_path UNIQUE (product_name, file_path);
 ^   ALTER TABLE ONLY public.tracked_files DROP CONSTRAINT pk_tracked_file_product_name_file_path;
       public            postgres    false    221    221         �           2606    16422    products products_pkey 
   CONSTRAINT     T   ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);
 @   ALTER TABLE ONLY public.products DROP CONSTRAINT products_pkey;
       public            postgres    false    217         �           1259    16408    ix_builds_product_name    INDEX     P   CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);
 *   DROP INDEX public.ix_builds_product_name;
       public            postgres    false    215         �           1259    17063    ix_products_name    INDEX     L   CREATE UNIQUE INDEX ix_products_name ON public.products USING btree (name);
 $   DROP INDEX public.ix_products_name;
       public            postgres    false    217                                                                                                                                                                                                                                      3360.dat                                                                                            0000600 0004000 0002000 00000000005 14414645602 0014246 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        \.


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           3356.dat                                                                                            0000600 0004000 0002000 00000006573 14414645602 0014273 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        2	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798639	us
3	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798639	eu
4	WOW-47657patch10.0.2_Retail	f1bc76dfaf6619e73133b56bfaaec3fd	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798639	cn
5	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798639	kr
6	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798640	tw
7	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798640	sg
8	WOW-48317patch10.0.5_Retail	e577645bdad65496e3f84135b244dfed	29289ea1234e55e6cab4ea011f2f4eef	wow	1677798640	xx
9	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	us
10	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	eu
11	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	cn
12	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	kr
13	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	tw
14	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	sg
15	WOW-47120patch10.0.2_Beta	155514ca94e87f373dfa8e244f591a58	29289ea1234e55e6cab4ea011f2f4eef	wow_beta	1677798641	xx
16	WOW-48340patch3.4.1_ClassicRetail	a57c0078be5af4493ff8428d84f32d5d	29289ea1234e55e6cab4ea011f2f4eef	wow_classic	1677798642	us
17	WOW-48340patch3.4.1_ClassicRetail	a57c0078be5af4493ff8428d84f32d5d	29289ea1234e55e6cab4ea011f2f4eef	wow_classic	1677798642	eu
18	WOW-47659patch3.4.0_ClassicRetail	e1d98cc146dfbec5a8430fc2b83c836e	29289ea1234e55e6cab4ea011f2f4eef	wow_classic	1677798642	cn
19	WOW-48340patch3.4.1_ClassicRetail	a57c0078be5af4493ff8428d84f32d5d	29289ea1234e55e6cab4ea011f2f4eef	wow_classic	1677798642	kr
20	WOW-48340patch3.4.1_ClassicRetail	a57c0078be5af4493ff8428d84f32d5d	29289ea1234e55e6cab4ea011f2f4eef	wow_classic	1677798642	tw
21	WOW-46158patch3.4.0_ClassicBeta	ccce0cc7e9e008ef5453a0b27fe84376	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_beta	1677798643	us
22	WOW-46158patch3.4.0_ClassicBeta	ccce0cc7e9e008ef5453a0b27fe84376	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_beta	1677798643	eu
23	WOW-46158patch3.4.0_ClassicBeta	ccce0cc7e9e008ef5453a0b27fe84376	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_beta	1677798643	kr
24	WOW-46158patch3.4.0_ClassicBeta	ccce0cc7e9e008ef5453a0b27fe84376	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_beta	1677798643	tw
25	WOW-48340patch3.4.1_ClassicPTR	7fe36b4479c5ffae15f9b6f0b6e3c152	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_ptr	1677798644	us
26	WOW-48340patch3.4.1_ClassicPTR	7fe36b4479c5ffae15f9b6f0b6e3c152	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_ptr	1677798644	eu
27	WOW-48340patch3.4.1_ClassicPTR	7fe36b4479c5ffae15f9b6f0b6e3c152	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_ptr	1677798644	cn
28	WOW-48340patch3.4.1_ClassicPTR	7fe36b4479c5ffae15f9b6f0b6e3c152	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_ptr	1677798644	kr
29	WOW-48340patch3.4.1_ClassicPTR	7fe36b4479c5ffae15f9b6f0b6e3c152	29289ea1234e55e6cab4ea011f2f4eef	wow_classic_ptr	1677798644	tw
\.


                                                                                                                                     3364.dat                                                                                            0000600 0004000 0002000 00000000005 14414645602 0014252 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        \.


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           3358.dat                                                                                            0000600 0004000 0002000 00000012455 14414645602 0014271 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        2	agent	1510339
3	agent_beta	1510338
5	anbsdev	1530242
7	auksa	1294658
9	auksdev	1378306
10	aukse	1482884
11	auksese	1482883
13	auksess	1482882
37	bna	1532930
38	brynhildr_odin	1531714
39	brynhildr_odin2	1531716
40	brynhildr_odin_qa	1527314
41	brynhildr_odin_qa2	1527315
42	brynhildr_odin_vendor	1527302
43	brynhildr_odin_vendor2	1527316
44	bts	1523650
46	d3	1521860
47	d3cn	1521859
48	d3t	1521858
49	fenrisdev	1410114
50	fenrisdev2	1004354
51	fenrise	1419330
55	forea	813122
58	forecdlstaff	1346627
59	foredev	813123
60	forev1	1526662
61	forev10	1526678
62	forev11	1526672
63	forev12	1526664
64	forev13	1526667
65	forev14	1526681
66	forev15	1526673
67	forev16	1526676
68	forev17	1526677
69	forev18	1526663
70	forev19	1526668
71	forev2	1526670
72	forev20	1526679
73	forev3	1526666
74	forev4	1526658
75	forev5	1526671
76	forev6	1526665
77	forev7	1526660
78	forev8	1526659
79	forev9	1526669
82	geirdrifulforecdlqa	1526674
83	geirdrifulforecdls	1346626
84	geirdrifulforecdlv	1526675
85	geirdrifulforeqa	1526680
86	geirdrifulforev	1526661
87	hero	1499650
88	heroc	1496899
89	herot	1496898
92	hsdev	671618
94	hse1	1467202
95	lazr	457026
96	lazrv1	453379
97	lazrv2	453378
119	odin	1531717
120	odina	1141058
121	odinb	1531715
122	odindev	1141059
123	odine	849026
124	odinv1	1527300
125	odinv10	1527310
126	odinv11	1527308
127	odinv12	1527313
128	odinv13	1527306
129	odinv14	1527303
130	odinv15	1527304
131	odinv16	1527317
132	odinv2	1527305
133	odinv3	1527312
134	odinv4	1527311
135	odinv5	1527309
136	odinv6	1527307
137	odinv7	1527301
138	odinv8	1527299
139	odinv9	1527298
140	orbisdev	357826
141	osi	1509188
142	osia	1247106
143	osib	1509187
144	osidev	1247107
145	osit	1509186
146	osiv1	1506690
147	osiv2	1507010
148	osiv3	1506692
149	osiv5	1506691
150	osiv6	1506693
108	moonv19	1540818
109	moonv2	1540814
110	moonv20	1540810
111	moonv21	1540867
112	moonv3	1540806
113	moonv4	1540821
115	moonv6	1540805
116	moonv7	1540819
117	moonv8	1540812
118	moonv9	1540807
15	auksv10	1540616
151	pro	1537986
19	auksv14	1540625
6	auks	1536130
36	auksv9	1540612
53	fenrisvendor2	1540419
102	moonv13	1540816
105	moonv16	1540804
27	auksv21	1540628
28	auksv22	1540627
30	auksv3	1540738
31	auksv4	1540623
32	auksv5	1540618
33	auksv6	1540622
34	auksv7	1540635
35	auksv8	1540610
20	auksv15	1540619
52	fenrisvendor	1540418
14	auksv1	1540634
90	hsb	1539331
17	auksv12	1540633
45	catalogs	1537602
103	moonv14	1540802
104	moonv15	1540808
21	auksv16	1540620
22	auksv17	1540636
23	auksv18	1540632
91	hsc	1539332
106	moonv17	1540813
26	auksv20	1540617
8	auksb	1533826
12	auksesp	1533829
25	auksv2	1540613
93	hse	1539333
100	moonv11	1540817
152	prob	1534469
153	proc	1534468
154	proc2	1534467
155	proc2_cn	1534477
156	proc2_eu	1534475
24	auksv19	1540615
99	moonv10	1540815
18	auksv13	1540621
54	fore	1537414
56	foreb	1537412
57	forec	1537410
81	geirdrifulforecdl	1537411
101	moonv12	1540820
114	moonv5	1540803
159	proc4	1520514
160	proc5	1520515
168	prodemo3	1520516
182	prov	1393794
183	provac	1393986
188	randgridaukscdls	1482885
192	rtro	526082
193	rtrodev	457922
196	s2	1485826
197	s2t	1485378
218	viper	1525890
219	viperdev	558274
220	viperv1	1520194
223	w3d	1437890
225	wlby	523010
226	wlbydev	497986
227	wlbyv1	518406
228	wlbyv2	518402
229	wlbyv3	518405
230	wlbyv4	518403
231	wlbyv6	518404
240	wowdev2	1031810
241	wowdev3	1502402
243	wowe3	357570
252	zeusa	373315
255	zeuscdlevent	677762
256	zeuscdlstaff	677763
257	zeusdev	373314
258	zeusevent	301378
259	zeusr	373316
4	anbs	1533058
221	w3	1532994
222	w3b	1532995
224	w3t	1532996
194	s1	1539843
236	wow_classic_era	1540042
166	prodemo	1540997
239	wowdev	1538178
210	spotv20	1539599
217	spotv9	1539603
173	proev	1534470
170	prodev6	1540999
171	prodev7	1540998
237	wow_classic_era_ptr	1540044
203	spotv14	1539593
261	zeusv10	1536650
262	zeusv11	1536642
263	zeusv12	1536643
251	zeus	1535876
253	zeusb	1535875
254	zeusc	1535874
238	wow_classic_ptr	1540036
174	proindev	1541002
245	wowt	1540037
195	s1t	1539842
211	spotv3	1539602
212	spotv4	1539591
80	geirdrifulfore	1537413
175	prolocv1	1540227
232	wow	1540034
198	spotv1	1539588
199	spotv10	1539594
213	spotv5	1539600
157	proc2_kr	1534476
161	proc_cn	1534474
162	proc_eu	1534472
163	proc_kr	1534473
164	procr	1534479
165	procr2	1534478
204	spotv15	1539592
205	spotv16	1539590
206	spotv17	1539596
200	spotv11	1539586
184	provbv	1540229
187	randgridaukscdlqa	1540626
189	randgridaukscdlv	1540630
190	randgridauksqa	1540624
179	proms	1534471
180	prot	1534480
201	spotv12	1539605
233	wow_beta	1540041
185	randgridauks	1533827
186	randgridaukscdl	1533828
277	prodev8	1540995
278	prodev9	1540996
172	prodevops	1541001
176	prolocv2	1540228
177	prolocv3	1540226
214	spotv6	1539601
215	spotv7	1539589
234	wow_classic	1540035
178	prolocv4	1540230
242	wowe1	1540040
207	spotv18	1539597
208	spotv19	1539598
202	spotv13	1539595
260	zeusv1	1536648
191	randgridauksv	1540631
246	wowv	1538243
264	zeusv13	1536656
265	zeusv14	1536653
266	zeusv15	1536649
267	zeusv16	1536657
268	zeusv2	1536644
269	zeusv3	1536645
270	zeusv4	1536647
271	zeusv5	1536651
272	zeusv6	1536655
273	zeusv7	1536646
274	zeusv8	1536652
275	zeusv9	1536654
167	prodemo2	1538371
169	prodev	1538370
247	wowv2	1538242
248	wowv3	1538245
249	wowv4	1538244
235	wow_classic_beta	1540039
250	wowz	1540038
209	spotv2	1539604
216	spotv8	1539587
16	auksv11	1540611
29	auksv23	1540629
98	moonv1	1540811
107	moonv18	1540809
276	moonv22	1540866
158	proc3	1540994
181	proutr	1541000
244	wowlivetest	1540043
\.


                                                                                                                                                                                                                   3362.dat                                                                                            0000600 0004000 0002000 00000000033 14414645602 0014251 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        1	wow	Wow.exe	Wow.exe
\.


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     restore.sql                                                                                         0000600 0004000 0002000 00000022717 14414645602 0015403 0                                                                                                    ustar 00postgres                        postgres                        0000000 0000000                                                                                                                                                                        --
-- NOTE:
--
-- File paths need to be edited. Search for $$PATH$$ and
-- replace it with the path to the directory containing
-- the extracted data files.
--
--
-- PostgreSQL database dump
--

-- Dumped from database version 15.1
-- Dumped by pg_dump version 15.1

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

DROP DATABASE tactmon;
--
-- Name: tactmon; Type: DATABASE; Schema: -; Owner: postgres
--

CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';


ALTER DATABASE tactmon OWNER TO postgres;

\connect tactmon

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

--
-- Name: DATABASE tactmon; Type: COMMENT; Schema: -; Owner: postgres
--

COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';


SET default_tablespace = '';

SET default_table_access_method = heap;

--
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
-- Name: bound_channels_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;


--
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
-- Name: COLUMN builds.detected_at; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';


--
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
-- Name: builds_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;


--
-- Name: command_states; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.command_states (
    id integer NOT NULL,
    name text NOT NULL,
    hash bigint NOT NULL,
    version integer NOT NULL
);


ALTER TABLE public.command_states OWNER TO postgres;

--
-- Name: command_states_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.command_states_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.command_states_id_seq OWNER TO postgres;

--
-- Name: command_states_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.command_states_id_seq OWNED BY public.command_states.id;


--
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
-- Name: products; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);


ALTER TABLE public.products OWNER TO postgres;

--
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
-- Name: tracked_file_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.tracked_file_id_seq OWNED BY public.tracked_files.id;


--
-- Name: bound_channels id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);


--
-- Name: builds id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);


--
-- Name: command_states id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.command_states ALTER COLUMN id SET DEFAULT nextval('public.command_states_id_seq'::regclass);


--
-- Name: tracked_files id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.tracked_files ALTER COLUMN id SET DEFAULT nextval('public.tracked_file_id_seq'::regclass);


--
-- Data for Name: bound_channels; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM stdin;
\.
COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM '$$PATH$$/3360.dat';

--
-- Data for Name: builds; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM stdin;
\.
COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM '$$PATH$$/3356.dat';

--
-- Data for Name: command_states; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.command_states (id, name, hash, version) FROM stdin;
\.
COPY public.command_states (id, name, hash, version) FROM '$$PATH$$/3364.dat';

--
-- Data for Name: products; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.products (id, name, sequence_id) FROM stdin;
\.
COPY public.products (id, name, sequence_id) FROM '$$PATH$$/3358.dat';

--
-- Data for Name: tracked_files; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.tracked_files (id, product_name, file_path, display_name) FROM stdin;
\.
COPY public.tracked_files (id, product_name, file_path, display_name) FROM '$$PATH$$/3362.dat';

--
-- Name: bound_channels_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.bound_channels_id_seq', 1, true);


--
-- Name: builds_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.builds_id_seq', 35, true);


--
-- Name: command_states_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.command_states_id_seq', 1, false);


--
-- Name: products_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.products_id_seq', 278, true);


--
-- Name: tracked_file_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.tracked_file_id_seq', 2, true);


--
-- Name: bound_channels bound_channels_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);


--
-- Name: builds builds_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);


--
-- Name: command_states command_state_uniq_name; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_state_uniq_name UNIQUE (name);


--
-- Name: command_states command_states_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_states_pkey PRIMARY KEY (id);


--
-- Name: tracked_files pk_tracked_file_product_name_file_path; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.tracked_files
    ADD CONSTRAINT pk_tracked_file_product_name_file_path UNIQUE (product_name, file_path);


--
-- Name: products products_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);


--
-- Name: ix_builds_product_name; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);


--
-- Name: ix_products_name; Type: INDEX; Schema: public; Owner: postgres
--

CREATE UNIQUE INDEX ix_products_name ON public.products USING btree (name);


--
-- PostgreSQL database dump complete
--

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 