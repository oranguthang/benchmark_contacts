import javax.persistence.Entity;
import javax.persistence.Id;
import java.time.LocalDateTime;

@Entity
public class Contact {

    @Id
    private Long id;
    private int externalId;
    private String phoneNumber;
    private LocalDateTime dateCreated;
    private LocalDateTime dateUpdated;

    // Getters and Setters
}
