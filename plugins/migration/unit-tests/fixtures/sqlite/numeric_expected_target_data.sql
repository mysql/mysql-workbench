/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `NumericContainer` (
  `id` int(11) NOT NULL DEFAULT '0',
  `fdata` float DEFAULT NULL,
  `ddata` double DEFAULT NULL,
  `ndata` decimal(10,0) DEFAULT NULL,
  `decdata` decimal(10,5) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
INSERT INTO `NumericContainer` VALUES (1,NULL,NULL,NULL,NULL),(2,0,0,0,0.00000),(3,0.00001,0.00001,0,0.00001),(4,-0.00001,-0.00001,0,-0.00001),(5,99.9999,99.9999,100,99999.99999);
