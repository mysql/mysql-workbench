-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------

-- -----------------------------------------------------
-- Table `table1`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `table1` ;

CREATE TABLE IF NOT EXISTS `table1` (
  `icolumn` INT NOT NULL AUTO_INCREMENT,
  `num` INT NOT NULL,
  PRIMARY KEY (`icolumn`))
ENGINE = InnoDB;


-- -----------------------------------------------------
-- procedure routine1
-- -----------------------------------------------------
DROP procedure IF EXISTS `routine1`;

DELIMITER $$
CREATE PROCEDURE `routine1` ()
BEGIN
	INSERT INTO `table1` ( `num` ) VALUES( ROUND( RAND() * 1000 ) );
END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure routine2
-- -----------------------------------------------------
DROP procedure IF EXISTS `routine2`;

DELIMITER $$
CREATE PROCEDURE `routine2` ()
BEGIN
	INSERT INTO `table1` ( `num` ) VALUES( ROUND( RAND() * 1000 ) + 1000 );
END$$

DELIMITER ;
SET SQL_MODE = '';
GRANT USAGE ON *.* TO user1;
 DROP USER user1;
SET SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';
CREATE USER 'user1';

GRANT ALL ON * TO 'user1';

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
