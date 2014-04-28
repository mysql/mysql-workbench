/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `StringContainer` (
  `id` int(11) NOT NULL DEFAULT '0',
  `str_data` longtext,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
INSERT INTO `StringContainer` VALUES (1,NULL),(2,''),(3,'A normal string with spaces and numb3r5'),(4,'\"`\';'),(5,'!$%&/()=?¡'),(6,'áéíóú àèìòù âêîôû äëïöü €¬');
