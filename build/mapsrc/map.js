function getMinecraftTile( coord, zoom )
{
	var zoomDivision = Math.pow( 2, zoom );
	var regionX, regionY;
	var xOffset, yOffset;
	var index;
	
	regionX = Math.floor( coord.x / zoomDivision );
	regionY = Math.floor( coord.y / zoomDivision );
	xOffset = Math.abs( coord.x % zoomDivision );
	yOffset = Math.abs( coord.y % zoomDivision );
	if( zoom != 0 )
	{
		if( coord.y < 0 )
			yOffset = zoomDivision - yOffset;
		if( coord.x < 0 )
			xOffset = zoomDivision - xOffset;
	}
	index = xOffset + (yOffset * zoomDivision);
	
	return "../maps/MCMapper Test/" + zoom + "/r." + regionX + "." + regionY + "-" + index + ".jpeg";
}

function setupMap()
{
	var mapDesc = {
		center:new google.maps.LatLng(0,0),
		zoom:0,
		streetViewControl: false,
		mapTypeControlOptions: {
			mapTypeIds: ['minecraft']
		}
	};
	
	var minecraftMap = new google.maps.Map( document.getElementById("minecraft-map"), mapDesc );
	
	var minecraftMapType = new google.maps.ImageMapType( {
		getTileUrl:getMinecraftTile,
		tileSize:new google.maps.Size( 512, 512 ),
		maxZoom:3,
		minZoom:0,
		radius:1738000,
		name:"Minecraft"
	});
	minecraftMap.mapTypes.set( "minecraft", minecraftMapType );
	minecraftMap.setMapTypeId( "minecraft" );
	
}
google.maps.event.addDomListener( window, 'load', setupMap );