-- ----------------------------------------------------------------------------
-- MySQL Workbench Migration
-- Migrated Schemata: sampledb
-- Source Schemata: C:\develop\6.0\fixtures\sampledb.sqlite
-- Created: Fri Apr 19 12:26:43 2013
-- ----------------------------------------------------------------------------

SET FOREIGN_KEY_CHECKS = 0;;

-- ----------------------------------------------------------------------------
-- Table sampledb.BlobContainer
-- ----------------------------------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sampledb`.`BlobContainer` (
  `id` INT NULL DEFAULT NULL ,
  `blob_data` LONGBLOB NULL DEFAULT NULL ,
  PRIMARY KEY (`id`) );
SET FOREIGN_KEY_CHECKS = 1;
