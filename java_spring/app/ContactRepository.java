import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.domain.PageRequest;

import java.util.List;

public interface ContactRepository extends JpaRepository<Contact, Long> {

    List<Contact> findByExternalIdAndPhoneNumber(
            Integer externalId,
            String phoneNumber,
            PageRequest pageRequest);

    default List<Contact> findByFilters(Integer externalId, String phoneNumber, PageRequest pageRequest) {
        if (externalId != null && phoneNumber != null) {
            return findByExternalIdAndPhoneNumber(externalId, phoneNumber, pageRequest);
        }
        // Implement other filter combinations
        return findAll(pageRequest).getContent();
    }
}
